#include "DriverMain.h"
#include "DriverFileOperations.h"
#include "ToolFunctions.h"

MODULE_LICENSE("Dual BSD/GPL");

struct SLDriverParameters gslDriverParameters = {0};

struct file_operations gslNvmDriverFileOperations = 
{
	.owner = THIS_MODULE,
	.open  = DriverOpen,
	.release = DriverClose,
	.read  = DriverRead,
	.write = DriverWrite,
	.unlocked_ioctl = DriverIOControl,
	.mmap = DriverMMap,
};

int InitalizeCharDevice(void)
{
	int result;
	struct device *pdevice;

	result = alloc_chrdev_region(&(gslDriverParameters.uiDeviceNumber), 0, 1, DEVICE_NAME);
	if(result < 0)
	{
		printk(KERN_ALERT DEVICE_NAME " alloc_chrdev_region error\n");
		return result;
	}

	gslDriverParameters.pslDriverClass = class_create(THIS_MODULE, DEVICE_NAME);
	if(IS_ERR(gslDriverParameters.pslDriverClass)) 
	{
		printk(KERN_ALERT DEVICE_NAME " class_create error\n");

		result = PTR_ERR(gslDriverParameters.pslDriverClass);
		goto CLASS_CREATE_ERROR;
	}

	cdev_init(&(gslDriverParameters.slCharDevice), &gslNvmDriverFileOperations);
	gslDriverParameters.slCharDevice.owner = THIS_MODULE;

	result = cdev_add(&(gslDriverParameters.slCharDevice), gslDriverParameters.uiDeviceNumber, 1);
	if(result < 0) 
	{
		printk(KERN_ALERT DEVICE_NAME " cdev_add error\n");
		goto CDEV_ADD_ERROR;
	}

	pdevice = device_create(gslDriverParameters.pslDriverClass, NULL, gslDriverParameters.uiDeviceNumber, NULL, DEVICE_NAME);
	if(IS_ERR(pdevice)) 
	{
		printk(KERN_ALERT DEVICE_NAME " device_create error\n");

		result = PTR_ERR(pdevice);
		goto DEVICE_CREATE_ERROR;
	}

	return 0;

DEVICE_CREATE_ERROR:
	cdev_del(&(gslDriverParameters.slCharDevice));

CDEV_ADD_ERROR:
	class_destroy(gslDriverParameters.pslDriverClass);

CLASS_CREATE_ERROR:
	unregister_chrdev_region(gslDriverParameters.uiDeviceNumber, 1);

	return result;
}

void UninitialCharDevice(void)
{
	device_destroy(gslDriverParameters.pslDriverClass, gslDriverParameters.uiDeviceNumber);

	cdev_del(&(gslDriverParameters.slCharDevice));

	class_destroy(gslDriverParameters.pslDriverClass);

	unregister_chrdev_region(gslDriverParameters.uiDeviceNumber, 1);
}

void PrintCompoudPageInfo(struct page *pCompoundPage)
{
	unsigned long ulMaskForPGhead, ulMaskForPGtail;

	ulMaskForPGhead = 1 << PG_head;
	ulMaskForPGtail = 1 << PG_tail;

	DEBUG_PRINT(DEVICE_NAME " page struct address = %lx\n", (unsigned long)pCompoundPage);

	DEBUG_PRINT(DEVICE_NAME " page->lru.prev = %lx\n", (unsigned long)pCompoundPage->lru.prev);

	if(pCompoundPage->flags & ulMaskForPGhead)
	{
		DEBUG_PRINT(DEVICE_NAME " set PG_head\n");
	}

	if(pCompoundPage->flags & ulMaskForPGtail)
	{
		DEBUG_PRINT(DEVICE_NAME " set PG_tail\n");
	}

	DEBUG_PRINT(DEVICE_NAME " page->first_page = %lx\n", (unsigned long)pCompoundPage->first_page);
}

void PrintCompoudPagesInfo(struct page *pCompoundPage)
{
	struct page *pNextPage;

	DEBUG_PRINT(DEVICE_NAME " \nThe first page struct:\n");
	PrintCompoudPageInfo(pCompoundPage);
	
	pNextPage = pCompoundPage + 1;
	DEBUG_PRINT(DEVICE_NAME " \nThe second page struct:\n");
	PrintCompoudPageInfo(pNextPage);

	pNextPage = pNextPage + 1;
	DEBUG_PRINT(DEVICE_NAME " \nThe third page struct:\n");
	PrintCompoudPageInfo(pNextPage);
}

static int DriverInitialize(void)
{
	struct page *pCompoundPage, *pNormalPage;

	SetPageReadAndWriteAttribute((unsigned long)DriverInitialize);

	DEBUG_PRINT(DEVICE_NAME " Initialize\n");

	DEBUG_PRINT(DEVICE_NAME " Compound page:\n");

	pCompoundPage = alloc_pages(__GFP_COMP, 5);

	PrintCompoudPagesInfo(pCompoundPage);

	__free_pages(pCompoundPage, 5);

	pNormalPage = alloc_pages(0, 1);

	DEBUG_PRINT(DEVICE_NAME " Normal page:\n");

	PrintCompoudPagesInfo(pNormalPage);

	__free_pages(pNormalPage, 1);

	return InitalizeCharDevice();
}

static void DriverUninitialize(void)
{
	DEBUG_PRINT(DEVICE_NAME " Uninitialize\n");

	UninitialCharDevice();
}

module_init(DriverInitialize);
module_exit(DriverUninitialize);
