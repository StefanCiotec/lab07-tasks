/*
 * SO2 Lab - Block device drivers (#7)
 * Linux - Exercise #4, #5 (Relay disk - bio)
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>

MODULE_AUTHOR("SO2");
MODULE_DESCRIPTION("Relay disk");
MODULE_LICENSE("GPL");

#define KERN_LOG_LEVEL		KERN_ALERT

#define PHYSICAL_DISK_NAME	"/dev/sdb"
#define KERNEL_SECTOR_SIZE	512

#define BIO_DIR_READ		0
#define BIO_DIR_WRITE		1

#define BIO_WRITE_MESSAGE	"def"


/* pointer to physical device structure */
static struct block_device *phys_bdev;

static void bi_complete(struct bio *bio, int error)
{
	int i;
	struct bio_vec *bvec;
	char *buf;

	/* TODO 4: read data (first 3 bytes) from bio buffer and print it */
	/* bio_for_each_segment_all(bvec, bio, i) {
		buf = __bio_kmap_atomic(bio, i);
		printk("%02x\n", buf[0]);
		printk("%02x\n", buf[1]);
		printk("%02x\n", buf[2]);
		__bio_kunmap_atomic(buf);
	} */
	/* TODO 4: complete bio */
	complete((struct completion*)bio->bi_private);

}

/* TODO 5: add direction parameter */
static void send_test_bio(struct block_device *bdev, int dir)
{
	struct bio *bio = bio_alloc(GFP_NOIO, 1);
	struct completion event;
	struct page *page;
	char *buf;
	char message[] = BIO_WRITE_MESSAGE;
	struct bio_vec *bvec;
	int i;

	/* TODO 4: fill bio (bdev, sector, endio, direction, completion) */
	init_completion(&event);

	bio->bi_bdev = bdev;
	bio->bi_sector = 0;
	bio->bi_private = &event;
	bio->bi_end_io = bi_complete;
	bio->bi_rw = dir;

	page = alloc_page(GFP_NOIO);
	bio_add_page(bio, page, KERNEL_SECTOR_SIZE, 0);
	bio->bi_vcnt = 1;
	bio->bi_idx = 0;

	/* TODO 5: write message to bio buffer if direction is write */
	if (dir == BIO_DIR_WRITE) {
		bio_for_each_segment_all(bvec, bio, i) {
			buf = __bio_kmap_atomic(bio,  i);
			memcpy(buf, message, sizeof(message));
			__bio_kunmap_atomic(buf);
		}
	}


	/* TODO 4: submit bio and wait for completion */
	submit_bio(dir, bio);
	wait_for_completion(&event);


	bio_put(bio);
	__free_page(page);
}

static struct block_device *open_disk(char *name)
{
	struct block_device *bdev;
	const fmode_t mode = FMODE_READ | FMODE_WRITE | FMODE_EXCL;

	/* TODO 4: get block device in exclusive mode */
	bdev = blkdev_get_by_path(name, mode, THIS_MODULE);

	return bdev;
}

static int __init relay_init(void)
{
	phys_bdev = open_disk(PHYSICAL_DISK_NAME);
	if (phys_bdev == NULL) {
		printk(KERN_ERR "[relay_init] No such device\n");
		return -EINVAL;
	}

	send_test_bio(phys_bdev, BIO_DIR_READ);

	return 0;
}

static void close_disk(struct block_device *bdev)
{
	/* TODO 4: put block device */
	const fmode_t mode = FMODE_READ | FMODE_WRITE | FMODE_EXCL;

	blkdev_put(bdev, mode);

}

static void __exit relay_exit(void)
{
	/* TODO 5: send test write bio */
	send_test_bio(phys_bdev, BIO_DIR_WRITE);
	close_disk(phys_bdev);
}

module_init(relay_init);
module_exit(relay_exit);
