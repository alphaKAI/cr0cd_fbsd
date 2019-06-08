#include <sys/types.h>
#include <sys/module.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/uio.h>

#define DEV_NAME "cr0cd"

static d_open_t cr0cd_open;
static d_close_t cr0cd_close;
static d_read_t cr0cd_read;
static d_write_t cr0cd_write;

/* Character device entry points */
static struct cdevsw cr0cd_cdevsw = {
    .d_version = D_VERSION,
    .d_open = cr0cd_open,
    .d_close = cr0cd_close,
    .d_read = cr0cd_read,
    .d_write = cr0cd_write,
    .d_name = DEV_NAME,
};

static struct cdev *cr0cd_dev;
volatile static int CR0;

static void int_to_bits(int n, char *bits) {
  int i;
  for (i = 0; i < 32; i++) {
    bits[i] = (n >> (31 - i)) & 1 ? '1' : '0';
  }
}

static int cr0cd_loader(struct module *m __unused, int what,
                        void *arg __unused) {
  int error = 0;

  switch (what) {
  case MOD_LOAD: /* kldload */
    error = make_dev_p(MAKEDEV_CHECKNAME | MAKEDEV_WAITOK, &cr0cd_dev,
                       &cr0cd_cdevsw, 0, UID_ROOT, GID_WHEEL, 0666, DEV_NAME);
    if (error != 0)
      break;

    printf("%s loaded.\n", DEV_NAME);
    break;
  case MOD_UNLOAD:
    destroy_dev(cr0cd_dev);
    printf("%s unloaded.\n", DEV_NAME);
    break;
  default:
    error = EOPNOTSUPP;
    break;
  }
  return (error);
}

static int cr0cd_open(struct cdev *dev __unused, int oflags __unused,
                      int devtype __unused, struct thread *td __unused) {
  int error = 0;

  uprintf("Opened %s successfully\n", DEV_NAME);
  return (error);
}

static int cr0cd_close(struct cdev *dev __unused, int fflag __unused,
                       int devtype __unused, struct thread *td __unused) {

  uprintf("Closing %s \n", DEV_NAME);
  return (0);
}

static int cr0cd_read(struct cdev *dev __unused, struct uio *uio,
                      int ioflag __unused) {
  int error;
  char bits[33];

  printf("%s is READ\n", DEV_NAME);

  asm("mov %%cr0, %%rax; mov %%rax, CR0" ::: "rax");
  int_to_bits(CR0, bits);
  bits[32] = '\0';

  if ((error = uiomove(bits, 33, uio)) != 0) {
    uprintf("uiomove failed!\n");
  }

  return error;
}

static int cr0cd_write(struct cdev *dev __unused, struct uio *uio,
                       int ioflag __unused) {
  int error;
  char bits[33];
  char mode[10];

  printf("%s IS WRITTEN\n", DEV_NAME);

  asm("mov %%cr0, %%rax; mov %%rax, CR0" ::: "rax"); // read

  bits[32] = '\0';
  int_to_bits(CR0, bits);
  printf("<BEFORE> CURRENT CR0: %s\n", bits);

  error = uiomove(mode, uio->uio_resid, uio);
  if (error != 0) {
    uprintf("Copy from user failed: bad address!\n");
    return error;
  }

  if (mode[0] == '1') { // disable
    printf("[DISABLE] CPU Cache\n");
    CR0 |= 1 << 30;
  } else {
    printf("[ENABLE] CPU Cache\n");
    CR0 &= ~(1 << 30);
  }

  asm("mov CR0, %%rax; mov %%rax, %%cr0" ::: "rax"); // write

  int_to_bits(CR0, bits);
  printf("<AFTER> CURRENT CR0: %s\n", bits);

  return error;
}

DEV_MODULE(cr0cd, cr0cd_loader, NULL);
