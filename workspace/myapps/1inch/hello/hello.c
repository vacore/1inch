#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <sys/random.h>
#include <unistd.h>

#include <nuttx/spi/spi_transfer.h>

#define ELOF(a) ((sizeof (a)/sizeof (a[0])))

#define LIM        10  // dimensions limit of the generated matrices
#define NTRANS     32  // number of SPI transmissions

mqd_t mq_matx,  // Queue1: RNG->Matrix multiplication
      mq_spi;   // Queue2: Matrix multiplication -> SPI Send
sem_t sem;

/* Multiply two matrices AA(X:Y), and BB(Y:Z), place the output into the matrix CC(X:Z) */
void matx_mul(int16_t *AA, int16_t *BB, int64_t *CC,
              size_t X, size_t Y, size_t Z) {
	int16_t (*A)[Y] = AA;  // len=X
	int16_t (*B)[Z] = BB;  // len=Y
	int64_t (*C)[Z] = CC;  // len=X

	for (size_t i = 0; i < X; i++) {
		for (size_t k = 0; k < Z; k++) {
			C[i][k] = 0;
			for (size_t j = 0; j < Y; j++) {
				C[i][k] += A[i][j] * B[j][k];
			}
		}
	}
}

/* SPI sending thread - fourth to highest prio */
void *thread_spi (void *arg) {
	int fd = open("/dev/spi1", O_RDONLY);
	assert (fd!=-1);

	pthread_cleanup_push ( ({ void _ (char *func) {
		close(fd);
		printf ("[%s]: done\n", func);
	}; _; }), __func__);

	struct mq_attr attr;
	mq_getattr(mq_spi, &attr);

	while (1) {
		size_t tot_sz = attr.mq_msgsize;
		uint8_t tot_buf[tot_sz];

		ssize_t len = mq_receive(mq_spi, tot_buf, tot_sz, NULL);
		assert(len > 0);

		printf("\e[46mResulting byte stream of len=%zu\e[49m\n", len);
		for (size_t i = 0; i < len; i++) {
			printf("0x%X ", tot_buf[i]);
		}
		putchar('\n');

		struct spi_trans_s trans = {
			.deselect = true,
			.nwords   = len,
			.txbuffer = tot_buf
		};
		struct spi_sequence_s seq = {
			.dev       = SPIDEV_ID(SPIDEVTYPE_USER, 0),
			.nbits     = 8,
			.ntrans    = 1,
			.frequency = 4000000,
			.trans     = &trans
		};

		int ret=ioctl (fd, SPIIOC_TRANSFER, (unsigned long)((uintptr_t)&seq));
		assert (ret!=-1);

		sem_post(&sem);
	}

	pthread_cleanup_pop (0);

	return NULL;
}

/* Matrix multiplication thread - third to highest prio */
void *thread_mul (void *arg) {
	struct mq_attr attr;
	mq_getattr(mq_matx, &attr);

	uint64_t cnt = 0;

	while (1) {
		size_t tot_sz = attr.mq_msgsize;
		uint8_t tot_buf[tot_sz];
		ssize_t len = mq_receive(mq_matx, tot_buf, tot_sz, NULL);

		uint8_t X = tot_buf[0],
		        Y = tot_buf[1],
		        Z = tot_buf[2];
		size_t dim_sz = sizeof(uint8_t) * 3;

		/* Multiply matrices */
		int16_t *AA = tot_buf+dim_sz;
		int16_t *BB = tot_buf+dim_sz + sizeof(int16_t) * (X * Y);

		size_t head_sz = sizeof cnt + dim_sz;
		size_t spi_sz = head_sz + sizeof(int64_t) * (X * Z);
		uint8_t spi_buf[spi_sz];

		uint8_t *p = spi_buf;
		*(uint64_t *)p = cnt++;
		p += sizeof(uint64_t);
		*p++ = X; *p++ = Y; *p++ = Z;

		int64_t *CC = p;
		matx_mul(AA, BB, CC, X, Y, Z);

		mq_send(mq_spi, spi_buf, spi_sz, 0);
	}

	return NULL;
}

/* Matrix thread - second to highest prio */
void *thread_matx (void *arg) {
	/* This thread's task - RNG matrix data */
	while (1) {
		/* Generate X,Y,Z (type uint8_t [1..LIM]) */
		size_t dim_sz = sizeof(uint8_t) * 3;
		unsigned char dim_buf[dim_sz];

		ssize_t n = getrandom(dim_buf, dim_sz, 0);
		assert (n==dim_sz);

		/* Make it in range [1..LIM] */
		for (size_t i = 0; i < dim_sz; i++) {
			dim_buf[i] %= LIM;
			dim_buf[i]++;
		}
		uint8_t X = dim_buf[0],
		        Y = dim_buf[1],
		        Z = dim_buf[2];

		/* Matrix data buffer */
		size_t matx_sz = sizeof(int16_t) * (X * Y + Y * Z),
		       tot_sz = dim_sz + matx_sz;
		char tot_buf[tot_sz];

		/* Copy dimensions */
		for (size_t i = 0; i < dim_sz; i++) {
			tot_buf[i] = dim_buf[i];
		}

		/* Fill the matrix values */
		n = getrandom(tot_buf+dim_sz, matx_sz, 0);
		assert (n==matx_sz);

		mq_send(mq_matx, tot_buf, tot_sz, 0);
		// usleep(10e3);
	}

	return NULL;
}

void matx (void) {
	sem_init (&sem,0,0);

	struct mq_attr attr_mtx = {
		.mq_maxmsg = 10,
		.mq_msgsize = sizeof (uint8_t) * 3                   // X,Y,Z
		            + sizeof(int16_t) * (LIM*LIM + LIM*LIM)  // Max dimensions limit
	};
	mq_matx = mq_open("/mq_matx", O_RDWR | O_CREAT, 0666, &attr_mtx);
	assert(mq_matx != -1);

	struct mq_attr attr_spi = {
		.mq_maxmsg = 10,
		.mq_msgsize = sizeof (uint8_t) * 2         // X,Z
		            + sizeof(int64_t) * (LIM*LIM)  // Max dimensions limit
	};
	mq_spi = mq_open("/mq_spi", O_RDWR|O_CREAT, 0666, &attr_spi);
	assert(mq_spi != -1);

	pthread_attr_t tattr;
	pthread_attr_init(&tattr);

	pthread_t tid_matx, tid_mul, tid_spi;

	#define MAIN_PRIO  CONFIG_ONEINCH_APPS_HELLO_PRIORITY
	struct {
		pthread_t tid;
		void *(*f)(void*);
		int prio;
	} t[] = {
		{ .f=thread_matx, .prio=MAIN_PRIO-3 },  // matrix generation thread
		{ .f=thread_mul,  .prio=MAIN_PRIO-2 },  // multiplication thread
		{ .f=thread_spi,  .prio=MAIN_PRIO-1 }   // spi thread
	};
	for (size_t i=0; i<ELOF (t); i++) {
		pthread_attr_setschedparam(&tattr, &(struct sched_param){.sched_priority = t[i].prio});
		pthread_create(&t[i].tid, &tattr, t[i].f, NULL);
	}

	/* Do specified amount of transmissions */
	uint8_t cnt = 0;
	while (cnt++ < NTRANS) {
		sem_wait (&sem);
	}

	for (size_t i=0; i<ELOF (t); i++) {
		pthread_cancel (t[i].tid);
		pthread_join (t[i].tid,NULL);
	}

	mq_close(mq_spi);
	mq_close(mq_matx);
	mq_unlink("/mq_spi");
	mq_unlink("/mq_matx");

	sem_destroy (&sem);
}

int main (int argc, char *argv[]) {
	puts("Hello, 1inch World!!");

	matx ();

	return 0;
}
