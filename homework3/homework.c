#include <minix/drivers.h>
#include <minix/chardriver.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <minix/ds.h>
#include <sys/ioc_homework.h>

#include "homework.h"

/*
 * Function prototypes for the hello driver.
 */
static int homework_open(devminor_t minor, int access, endpoint_t user_endpt);
static int homework_close(devminor_t minor);
static ssize_t homework_read(devminor_t minor, u64_t position, endpoint_t endpt,
    cp_grant_id_t grant, size_t size, int flags, cdev_id_t id);
static ssize_t homework_write(devminor_t minor, u64_t position, endpoint_t endpt,
    cp_grant_id_t grant, size_t size, int flags, cdev_id_t id);
static int homework_ioctl(devminor_t minor, unsigned long request, 
    endpoint_t endpt, cp_grant_id_t grant, int flags, endpoint_t user_endpt, cdev_id_t id);   

/* SEF functions and variables. */
static void sef_local_startup(void);
static int sef_cb_init(int type, sef_init_info_t *info);
static int sef_cb_lu_state_save(int);
static int lu_state_restore(void);

/* Entry points to the homework driver. */
static struct chardriver homework_tab =
{
    .cdr_open	= homework_open,
    .cdr_close	= homework_close,
    .cdr_read	= homework_read,
    .cdr_write  = homework_write,
    .cdr_ioctl  = homework_ioctl,
};

/** State variable to count the number of times the device has been opened.
 * Note that this is not the regular type of open counter: it never decreases.
 */
static int open_counter;

/* initialize slots in dev/homework to 0 */
static u32_t slots[5] = { 0 };

/* set the inital slot with the integer value */
static u32_t current_slot = 0;

/* presumed state variable for read/write size */
static const size_t int_size = sizeof(current_slot);

/* validity of slot */
static int valid_slot[5] = {0};

/* struct for a blocked process */
struct info_blocked_proc
{
    endpoint_t endpt;
    cdev_id_t caller_id;
    cp_grant_id_t grant;
};
typedef struct info_blocked_proc blk_proc_info;

/* array of processes blocked by IO slot */
static blk_proc_info blockedprocesses[5][5]; 

/* blocked processes for each slot */
static int num_blocked_procs[5] = {0};


static int homework_open(devminor_t UNUSED(minor), int UNUSED(access),
    endpoint_t UNUSED(user_endpt))
{
    printf("homework_open(). Called %d time(s).\n", ++open_counter);
    return OK;
}

static int homework_close(devminor_t UNUSED(minor))
{
    printf("homework_close()\n");
    return OK;
}

static ssize_t homework_read(devminor_t UNUSED(minor), u64_t UNUSED(position),
    endpoint_t endpt, cp_grant_id_t grant, size_t size, int UNUSED(flags),
    cdev_id_t id)
{
    size_t dev_size;
    u32_t *ptr = slots + current_slot;
    int ret;

    printf("homework_read()\n");

    /* This is the total size of our device. */
    dev_size = 4; 

    /* Check for invalid arg, and possibly limit the read size. */
    if (size < dev_size) return EINVAL;		/* invalid argumenr errno */
    if (size > dev_size)
        size = (size_t)(dev_size);	/* silently limit size */

    if(valid_slot[current_slot] == 0)
    {
        if(num_blocked_procs[current_slot] < 5)
	{
            blockedprocesses[current_slot][num_blocked_procs[current_slot]].endpt = endpt;
	    blockedprocesses[current_slot][num_blocked_procs[current_slot]].grant = grant;
	    blockedprocesses[current_slot][num_blocked_procs[current_slot]].caller_id = id;
	    num_blocked_procs[current_slot]++;

	    return EDONTREPLY;
	}
	else
	{
            return EWOULDBLOCK;
	}
    }

    /* Copy the requested part to the caller. */
    if ((ret = sys_safecopyto(endpt, grant, 0, (vir_bytes) ptr, int_size)) != OK)
        return ret;

    /* Return the number of bytes read. */
    return int_size;
}

static ssize_t homework_write(devminor_t UNUSED(minor), u64_t position,
    endpoint_t endpt, cp_grant_id_t grant, size_t size, int UNUSED(flags),
	cdev_id_t UNUSED(id))
{
    size_t dev_size;
    u32_t *ptr = slots + current_slot;
    int ret;

    printf("homework_write()\n");

    /* This is the total size of our device. */
    dev_size = 4;

    /* Check for invalid arg, and possibly limit the read size. */
    if (size < dev_size) return EINVAL;         /* invalid argumenr errno */
    if (size > dev_size)
        size = (size_t)(dev_size);      /* silently limit size */

    /* Copy the provided part from the caller. */
    if ((ret = sys_safecopyfrom(endpt, grant, 0, (vir_bytes) ptr, int_size)) != OK)
        return ret;

    /* check blocked processes trying to read; wake and read */
    if(valid_slot[current_slot] && num_blocked_procs[current_slot] > 0)
    {
        for(int n = 0; n < num_blocked_procs[current_slot]; n++)
	{
            sys_safecopyto(
			   blockedprocesses[current_slot][n].endpt,
			   blockedprocesses[current_slot][n].grant,
			   0,
			   (vir_bytes)ptr,
			   int_size
			   );
            chardriver_reply_task(
				  blockedprocesses[current_slot][n].endpt,
				  blockedprocesses[current_slot][n].caller_id,
				  current_slot
				  );
	}
    }
    valid_slot[current_slot] = 1;

    /* Return the number of bytes written. */
    return int_size;
}


static int homework_ioctl(devminor_t minor, unsigned long request,
    endpoint_t endpt, cp_grant_id_t grant, int flags, endpoint_t user_endpt, cdev_id_t id)
{
    u32_t temp = current_slot;

    if(request == HIOCSLOT) {
	printf("homework_ioctl() -> HIOCSLOT\n");

	/* set input to slot value */
	sys_safecopyfrom(endpt, grant, 0, (vir_bytes) &temp, int_size);
	if(temp > 4) {
		printf("HIOCSLOT: slot must be 0 to 4\n");
		return EINVAL;
	}
	
	current_slot = temp;
	printf("HIOCSLOT: slot in use = %d\n", current_slot);

	return EXIT_SUCCESS;
    }	
    if(request == HIOCCLEARSLOT) {
	printf("homework_ioctl() -> HIOCCLEARSLOT\n");

	/* clear the input slot */
	sys_safecopyfrom(endpt, grant, 0, (vir_bytes) &temp, int_size);
	if(temp > 4) {
		printf("HIOCCLEARSLOT: slot must be 0 to 4\n");
		return EINVAL;
	}
	slots[temp] = 0;
	printf("HIOCCLEARSLOT: slot cleared(%d) value: %d\n", temp, slots[temp]);

	return EXIT_SUCCESS;
    }
    if(request == HIOCGETSLOT) {
	printf("homework_ioctl() -> HIOCGETSLOT\n");

	/** return the current slot */
	sys_safecopyto(endpt, grant, 0, (vir_bytes) &current_slot, int_size);
	printf("HIOCGETSLOT: slot returned: %d\n", temp);

	return EXIT_SUCCESS;
    }

    return ENOTTY;
}

static int sef_cb_lu_state_save(int UNUSED(state)) {
/* Save the state. */
    ds_publish_u32("open_counter", open_counter, DSF_OVERWRITE);

    return OK;
}

static int lu_state_restore() {
/* Restore the state. */
    u32_t value;

    ds_retrieve_u32("open_counter", &value);
    ds_delete_u32("open_counter");
    open_counter = (int) value;

    return OK;
}

static void sef_local_startup()
{
    /*
     * Register init callbacks. Use the same function for all event types
     */
    sef_setcb_init_fresh(sef_cb_init);
    sef_setcb_init_lu(sef_cb_init);
    sef_setcb_init_restart(sef_cb_init);

    /*
     * Register live update callbacks.
     */
    /* - Agree to update immediately when LU is requested in a valid state. */
    sef_setcb_lu_prepare(sef_cb_lu_prepare_always_ready);
    /* - Support live update starting from any standard state. */
    sef_setcb_lu_state_isvalid(sef_cb_lu_state_isvalid_standard);
    /* - Register a custom routine to save the state. */
    sef_setcb_lu_state_save(sef_cb_lu_state_save);

    /* Let SEF perform startup. */
    sef_startup();
}

static int sef_cb_init(int type, sef_init_info_t *UNUSED(info))
{
/* Initialize the homework driver. */
    int do_announce_driver = TRUE;

    open_counter = 0;
    switch(type) {
        case SEF_INIT_FRESH:
            printf("%s", HOMEWORK_MESSAGE);
        break;

        case SEF_INIT_LU:
            /* Restore the state. */
            lu_state_restore();
            do_announce_driver = FALSE;

            printf("%sHey, I'm a new version!\n", HOMEWORK_MESSAGE);
        break;

        case SEF_INIT_RESTART:
            printf("%sHey, I've just been restarted!\n", HOMEWORK_MESSAGE);
        break;
    }

    /* Announce we are up when necessary. */
    if (do_announce_driver) {
        chardriver_announce();
    }

    /* Initialization completed successfully. */
    return OK;
}

int main(void)
{
    /*
     * Perform initialization.
     */
    sef_local_startup();

    /*
     * Run the main loop.
     */
    chardriver_task(&homework_tab);
    return OK;
}

