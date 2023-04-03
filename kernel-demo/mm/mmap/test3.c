    // up_read(&mm->mmap_sem);

    if (ret) {
        printk("remap_pfn_range failed, vm_start: 0x%lx\n", vma->vm_start);
    }
    else {
        printk("map kernel 0x%px to user 0x%lx, size: 0x%lx\n",
               virt_start, vma->vm_start, size);
    }

    return ret;
}


static loff_t demodev_llseek(struct file *file, loff_t offset, int whence)
{
    loff_t pos;
    switch(whence) {
    case 0: /* SEEK_SET */
        pos = offset;
        break;
    case 1: /* SEEK_CUR */
        pos = file->f_pos + offset;
        break;
    case 2: /* SEEK_END */
        pos = MAX_SIZE + offset;
        break;
    default:
        return -EINVAL;
    }
    if (pos < 0 || pos > MAX_SIZE)
        return -EINVAL;

    file->f_pos = pos; 
    return pos;
}
