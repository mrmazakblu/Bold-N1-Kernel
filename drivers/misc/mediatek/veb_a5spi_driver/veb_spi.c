#include <linux/spi/spi.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>

#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
//#include <mtk_spi.h>



#include "veb_common.h"
#include "veb_platform.h"
#include "veb_errno.h"
#include "veb_crc.h"
#include "veb_base.h"
#include "veb_a5.h"

#include "veb_a5_upgrade.h"

typedef struct
{
    int len;
    int iCrc;
    unsigned char *recv;
    unsigned char b_con;
}a5_duplex_con;

const char veb_a5_name[] = VEB_A5_NAME;
#if 0
static struct miscdevice veb_a5_miscdev = {
    MISC_DYNAMIC_MINOR,
    VEB_A5_NAME,
    &veb_a5_fops
};
#endif

typedef struct veb_data {
    struct miscdevice misc;
    veb_private_data_t priv;
} veb_data_t;

unsigned int irq_gpio;

static long veb_ioctl_duplex(veb_private_data_t *priv, a5_duplex_con *con)
{
    int ret = VEB_OK;
    struct spi_transfer xfer, xfer_left;
    struct spi_message p;
    unsigned char *tx_tmp = NULL;
    unsigned char *rx_tmp = NULL;

    int count, left, length;
    unsigned char *tx_left, *rx_left;

    count = con->len;
    if(count < (0x40000 + 0x400)){
        left = count % 1024;
        length = count - left;
    }else if(count == (0x40000 + 0x400)){
        left = 0x400;
        length = 0x40000;
    }else{
        VEB_ERR("a5_ioctl data should not over 256K+1K\n");
        return VEB_ERR_PARAM;
    }

    mutex_lock(&(priv->dev_lock));
    ret = veb_wait_ready();
    if(VEB_OK != ret)
    {
        mutex_unlock(&(priv->dev_lock));
        VEB_ERR("Waiting for ready failed!\n");
        return VEB_ERR_WAIT_BUSY_TIMEOUT;
    }

    mutex_lock(&(priv->mmap_lock));
    tx_tmp = (unsigned char*)priv->mmap_addr;
    rx_tmp = (unsigned char *)(priv->mmap_addr + (VEB_MMAP_SIZE >> 1));

    spi_message_init(&p);
    xfer.tx_buf     = tx_tmp;
    xfer.rx_buf     = rx_tmp;
    xfer.len        = length;
    xfer.cs_change  = 0;
    xfer.bits_per_word = VEB_SPI_BITS_PER_WORD;
    xfer.delay_usecs = 3;
    spi_message_add_tail(&xfer, &p);

    if(left){
        tx_left = tx_tmp + length;
        rx_left = rx_tmp + length;

        xfer_left.tx_buf     = tx_left;
        xfer_left.rx_buf     = rx_left;
        xfer_left.len        = left;
        xfer_left.cs_change  = 0;
        xfer_left.bits_per_word = VEB_SPI_BITS_PER_WORD;
        xfer_left.delay_usecs = 3;
        spi_message_add_tail(&xfer_left, &p);
    }

    spi_sync(priv->spi, &p);
    mutex_unlock(&(priv->mmap_lock));
    mutex_unlock(&(priv->dev_lock));

    return ret;
}

static long veb_ioctl_config(veb_private_data_t *priv, unsigned int cmd, unsigned long arg)
{
    cmd_t cmdt;
    long ret = VEB_OK;

    if(copy_from_user(&cmdt, (void __user *)arg, sizeof(cmd_t)))
    {
        VEB_ERR("copy from user failed!\n");
        return VEB_ERR_COPY_FROM_USER;
    }

    mutex_lock(&(priv->dev_lock));
    ret = veb_cmd_config(priv->spi, &(cmdt));
    mutex_unlock(&(priv->dev_lock));
    if(VEB_OK != ret)
    {
        VEB_ERR("veb_cmd_config failed(%ld)!\n", ret);
        VEB_EDUMP_WITH_PREFIX("command", ((char *)&(cmdt)), CMD_T_SIZE);
        return ret;
    }

    VEB_TRACE_OUT();
    return ret;
}

static long veb_ioctl_read(veb_private_data_t *priv, unsigned int cmd, unsigned long arg)
{
    struct veb_sdata s;
    long ret = VEB_OK;
    char *buf = NULL;

    if(copy_from_user(&s, (void __user *)arg, sizeof(struct veb_sdata)))
    {
        VEB_ERR("copy from user failed!\n");
        return VEB_ERR_COPY_FROM_USER;
    }

    if (s.type == VEB_DATA_MMAP)
    {
        mutex_lock(&(priv->mmap_lock));
        buf = priv->mmap_addr;
    }
    else if((s.type == VEB_DATA_COPY) && (s.s != NULL))
    {
        buf = kzalloc(s.length, GFP_KERNEL);
    }
    else
    {
        VEB_ERR("type(%d) or s of struct veb_sdata is illegal!\n", s.type);
        return VEB_ERR_PARAM;
    }

    mutex_lock(&(priv->dev_lock));
    ret = veb_cmd_read(priv->spi, &(s.cmd), buf, s.length);
    mutex_unlock(&(priv->dev_lock));

    if (VEB_OK != ret)
    {
        VEB_ERR("veb_cmd_read(length:%d) failed(%ld)!\n", s.length, ret);
        VEB_EDUMP_WITH_PREFIX("command", ((char *)&(s.cmd)), CMD_T_SIZE);
        goto error;
    }

    if (s.type == VEB_DATA_COPY)
    {
        if (copy_to_user((void __user *)s.s, buf, s.length))
        {
            VEB_ERR("copy to user failed!\n");
            ret = VEB_ERR_COPY_TO_USER;
            goto error;
        }
    }
error:
    if (s.type == VEB_DATA_MMAP)
    {
        mutex_unlock(&(priv->mmap_lock));
    } else {
        kfree(buf);
    }
    VEB_TRACE_OUT();
    return ret;
}

static long veb_ioctl_write(veb_private_data_t *priv, unsigned int cmd, unsigned long arg)
{
    veb_sdata_t s;
    long ret = VEB_OK;
    char *buf = NULL;

    if(copy_from_user(&s, (void __user *)arg, sizeof(struct veb_sdata)))
    {
        VEB_ERR("copy from user failed!\n");
        return VEB_ERR_COPY_FROM_USER;
    }

    if(s.type == VEB_DATA_MMAP)
    {
        mutex_lock(&(priv->mmap_lock));
        buf = priv->mmap_addr;
    }
    else if((s.type == VEB_DATA_COPY) && (s.s != NULL))
    {
        buf = kzalloc(s.length, GFP_KERNEL);
        if (NULL == buf)
        {
            VEB_ERR("kzalloc(size:%d) failed!\n", s.length);
            return VEB_ERR_MALLOC;
        }

        if (copy_from_user(buf, s.s, s.length))
        {
            kfree(buf);
            VEB_ERR("copy from user failed!\n");
            return VEB_ERR_COPY_FROM_USER;
        }
    }
    else
    {
        VEB_ERR("type(%d) or s of struct veb_sdata is illegal!\n", s.type);
        return VEB_ERR_PARAM;
    }

    mutex_lock(&(priv->dev_lock));
    ret = veb_cmd_write(priv->spi, &(s.cmd), buf, s.length);
    mutex_unlock(&(priv->dev_lock));
    if (s.type == VEB_DATA_MMAP)
    {
        mutex_unlock(&(priv->mmap_lock));
    }
    else
    {
        kfree(buf);
    }

    if (VEB_OK != ret)
    {
        VEB_ERR("veb_cmd_write(length:%d) failed(%ld)!\n", s.length, ret);
        VEB_EDUMP_WITH_PREFIX("command", ((char *)&(s.cmd)), CMD_T_SIZE);
    }
    VEB_TRACE_OUT();
    return ret;
}

static long veb_ioctl_exchange(veb_private_data_t *priv, unsigned int cmd, unsigned long arg)
{
    veb_ddata_t d;
    long ret = VEB_OK;

    veb_mmap_data_t *m = NULL;

    if(copy_from_user(&d, (void __user *)arg, sizeof(veb_ddata_t)))
    {
        VEB_ERR("copy from user failed!\n");
        return VEB_ERR_COPY_FROM_USER;
    }

    if(d.type == VEB_DATA_MMAP)
    {
        mutex_lock(&(priv->mmap_lock));
        m = (veb_mmap_data_t *)(priv->mmap_addr);
    }
    else if((d.type == VEB_DATA_COPY) && (d.source != NULL) && (d.sink != NULL))
    {
        if(d.srLength > VEB_FRAME_MAX)
        {
            VEB_ERR("length(source:%d) is illegal!\n", d.srLength);
            return VEB_ERR_LEN;
        }

        if(d.skLength > VEB_FRAME_MAX)
        {
            VEB_ERR("length(sink:%d) is illegal!\n", d.skLength);
            return VEB_ERR_LEN;
        }

        m = kzalloc(sizeof(veb_mmap_data_t), GFP_KERNEL);
        if(NULL == m)
        {
            VEB_ERR("kzalloc(size:%d) failed!\n", ((int)sizeof(veb_mmap_data_t)));
            return VEB_ERR_MALLOC;
        }

        if(copy_from_user(m->source, d.source, d.srLength))
        {
            kfree(m);
            VEB_ERR("copy from user failed!\n");
            return VEB_ERR_COPY_FROM_USER;
        }
    }
    else
    {
        VEB_ERR("type(%d) or s of struct veb_sdata is illegal!\n", d.type);
        return VEB_ERR_PARAM;
    }

    mutex_lock(&(priv->dev_lock));
    ret = veb_cmd_exchange(priv->spi, &(d.cmd), m->source, d.srLength, m->sink, d.skLength);
    mutex_unlock(&(priv->dev_lock));
    if (d.type == VEB_DATA_MMAP)
    {
        mutex_unlock(&(priv->mmap_lock));
    }
    else if (d.type == VEB_DATA_COPY)
    {
        if (VEB_OK == ret) {
            if (copy_to_user(d.sink, m->sink, d.skLength))
            {
                kfree(m);
                VEB_ERR("copy from user failed!\n");
                return VEB_ERR_COPY_FROM_USER;
            }
        }
        kfree(m);
    }
    else
    {
        VEB_ERR("veb_cmd_exchange(source:%d) failed(%ld)!\n", d.srLength, ret);
        VEB_EDUMP_WITH_PREFIX("command", ((char *)&(d.cmd)), CMD_T_SIZE);
    }

    VEB_TRACE_OUT();
    return ret;
}

static long veb_ioctl_restore_mode(struct veb_private_data *priv, unsigned int cmd, unsigned long arg)
{
    int type = 0;
    long ret = VEB_OK;
    if(copy_from_user(&type, (void __user *)arg, sizeof(int)))
    {
        VEB_ERR("copy from user failed!\n");
        return VEB_ERR_COPY_FROM_USER;
    }
    VEB_ERR("veb_ioctl_restore_mode 0:boot 1:rom type = %d!\n", type);
    mutex_lock(&(priv->dev_lock));
    ret = veb_a5_restore_mode(priv, type);
    mutex_unlock(&(priv->dev_lock));
    if (VEB_OK != ret)
    {
        VEB_ERR("veb_ioctl_restore_mode(type:%d) failed(%ld)!\n", type, ret);
        goto error;
    }

error:
    VEB_TRACE_OUT();
    return ret;
}


static long veb_ioctl_upgrade(struct veb_private_data *priv, unsigned int cmd, unsigned long arg)
{
    struct veb_cos c;
    long ret = VEB_OK;

    char *buf = NULL;

    if(copy_from_user(&c, (void __user *)arg, sizeof(struct veb_cos)))
    {
        VEB_ERR("copy from user failed!\n");
        return VEB_ERR_COPY_FROM_USER;
    }
    VEB_DDUMP_WITH_PREFIX("struct veb_cos :", (unsigned char *)&c, sizeof(struct veb_cos));

    if(c.type == VEB_DATA_MMAP)
    {
        mutex_lock(&(priv->mmap_lock));
        buf = priv->mmap_addr;
        VEB_DBG("mmap_addr = %p\n", priv->mmap_addr);
        VEB_DDUMP_WITH_PREFIX("cos :", buf, 48);
    }
    else if((c.type == VEB_DATA_COPY) && (c.s != NULL))
    {
        buf = kzalloc(c.length, GFP_KERNEL);
        if(copy_from_user(buf, c.s, c.length))
        {
            kfree(buf);
            VEB_ERR("copy from user failed!\n");
            return VEB_ERR_COPY_FROM_USER;
        }
        VEB_DDUMP_WITH_PREFIX("cos :", buf, 48);
    }
    else
    {
        VEB_ERR("type(%d) or s of struct veb_sdata is illegal!\n", c.type);
        return VEB_ERR_PARAM;
    }

    mutex_lock(&(priv->dev_lock));
    ret = veb_a5_upgrade(priv, c.cos_type, buf, c.length);
    mutex_unlock(&(priv->dev_lock));
    if (VEB_OK != ret)
    {
        VEB_ERR("upgrade(length:%d) failed(%ld)!\n", c.length, ret);
        goto error;
    }

error:
    if(c.type == VEB_DATA_MMAP)
    {
        mutex_unlock(&(priv->mmap_lock));
    }
    else
    {
        kfree(buf);
    }

    VEB_TRACE_OUT();
    return ret;
}


int veb_ioctl_crypto(veb_private_data_t *priv, unsigned int cmd, unsigned long arg)
{
    unsigned char *IV = NULL, tmp[IV_LEN];
    int ret = VEB_OK;
    veb_crypto_data_t data;
    char *buf = NULL;

    VEB_TRACE_IN();
    memset(&data, 0, sizeof(data));
    if(copy_from_user(&data, (void __user *)arg, sizeof(veb_crypto_data_t)))
    {
        VEB_ERR("copy from user failed!\n");
        return VEB_ERR_COPY_FROM_USER;
    }

    if (data.iv != NULL)
    {
        IV = tmp;
        if(copy_from_user(IV, data.iv, IV_LEN))
        {
            VEB_ERR(" copy_from_user--- error!\n");
            return VEB_ERR_COPY_FROM_USER;
        }
    }

    if (data.type == VEB_DATA_MMAP)
    {
        mutex_lock(&(priv->mmap_lock));
        buf = priv->mmap_addr;
    }
    else if((data.type == VEB_DATA_COPY) && (data.inbuf != NULL))
    {
        int buf_len;

        buf_len = data.inlen + 128;
        if (VEB_CMD_SYNC_DUPLEX_CRYPTO == cmd)
            buf_len += 3 * XFER_BLOCK_SIZE;
        buf = kzalloc(buf_len, GFP_KERNEL);
        if (buf == NULL)
        {
            VEB_ERR("buf null nomen \n");
            return -ENOMEM;
        }
        if (copy_from_user(buf, data.inbuf, data.inlen))
        {
            kfree(buf);
            VEB_ERR(" copy_from_user+++ error!\n");
            return VEB_ERR_COPY_FROM_USER;
        }
    }
    else
    {
        VEB_ERR("type(%d) or s of struct veb_sdata is illegal!\n", data.type);
        return VEB_ERR_PARAM;
    }

    mutex_lock(&(priv->dev_lock));
    switch (cmd) 
    {
    case VEB_CMD_SYNC_CRYPTO:
        ret = veb_sym_crypto(priv->spi, &(data.cmd), buf, IV, data.inlen);
        break;

    case VEB_CMD_SYNC_DUPLEX_CRYPTO:
        ret = veb_sym_duplex_crypto(priv->spi, &(data.cmd), buf, IV, data.inlen);
        break;

    default:
        ret = VEB_ERR_IOCTL;
        break;
    }
    mutex_unlock(&(priv->dev_lock));
    if (!!ret)
    {
        VEB_ERR("crpyto cmd %d fail \n", cmd);
        goto error;
    }
    if (data.type == VEB_DATA_COPY) {
      if (copy_to_user(((veb_crypto_data_t *)arg)->outbuf, buf, data.outlen))
      {
          VEB_ERR(" copy_to_user error!\n");
          ret = VEB_ERR_COPY_TO_USER;
      }
    }

error:
    if (data.type == VEB_DATA_MMAP)
    {
        mutex_unlock(&(priv->mmap_lock));
    }
    else {
        kfree(buf);
    }

    return ret;
}



int veb_compat_ioctl_crypto(veb_private_data_t *priv, unsigned int cmd, unsigned long arg)
{
    unsigned char *IV = NULL, tmp[IV_LEN];
    int ret = VEB_OK;
    veb_crypto_data_t data;
    char *buf = NULL;
    veb_crypto_com_data_t __user * p_com_data =NULL;
    compat_uptr_t p;
    int err = 0;
    
    VEB_TRACE_IN();
    memset(&data, 0, sizeof(data));
    if(copy_from_user(&data, (void __user *)arg, sizeof(cmd_t)))
    {
        VEB_ERR("copy from user failed!\n");
        return VEB_ERR_COPY_FROM_USER;
    }
    p_com_data = (veb_crypto_com_data_t __user *)arg;
    //err = get_user(data.cmd, &(p_com_data->cmd));
    err |= get_user(data.type, &(p_com_data->type));
    err |= get_user(data.inlen, &(p_com_data->inlen));
    err |= get_user(p, &(p_com_data->inbuf));
    data.inbuf = compat_ptr(p);
    err |= get_user(data.outlen, &(p_com_data->outlen));
    err |= get_user(p, &(p_com_data->outbuf));
    data.outbuf = compat_ptr(p);
    err |= get_user(p, &(p_com_data->iv));
    data.iv = compat_ptr(p);
    if(err)
     {
        VEB_ERR("get user failed!\n");
        return VEB_ERR_COPY_FROM_USER;
     }
    VEB_ERR("inlen = %d\n", data.inlen);
    VEB_ERR("outlen = %d\n", data.outlen);
    if (data.iv != NULL)
    {
        IV = tmp;
        
        if(copy_from_user(IV, data.iv, IV_LEN))
        {
            VEB_ERR(" copy_from_user--- error!\n");
            return VEB_ERR_COPY_FROM_USER;
        }
    }

    if (data.type == VEB_DATA_MMAP)
    {
        mutex_lock(&(priv->mmap_lock));
        buf = priv->mmap_addr;
    }
    else if((data.type == VEB_DATA_COPY) && (data.inbuf != NULL))
    {
        int buf_len;

        buf_len = data.inlen + 128;
        if (VEB_CMD_SYNC_DUPLEX_CRYPTO == cmd)
            buf_len += 3 * XFER_BLOCK_SIZE;
        buf = kzalloc(buf_len, GFP_KERNEL);
        if (buf == NULL)
        {
            VEB_ERR("buf null nomen \n");
            return -ENOMEM;
        }
        VEB_ERR("inbuf= %ld!\n",sizeof(data.inbuf));
        if (copy_from_user(buf, data.inbuf, data.inlen))
        {
            kfree(buf);
            VEB_ERR(" copy_from_user+++ error!\n");
            return VEB_ERR_COPY_FROM_USER;
        }
        VEB_DDUMP_WITH_PREFIX("buf:", ((unsigned char *)buf), data.inlen);
    }
    else
    {
        VEB_ERR("type(%d) or s of struct veb_sdata is illegal!\n", data.type);
        return VEB_ERR_PARAM;
    }

    mutex_lock(&(priv->dev_lock));
    switch (cmd) 
    {
    case VEB_CMD_SYNC_CRYPTO:
        ret = veb_sym_crypto(priv->spi, &(data.cmd), buf, IV, data.inlen);
        break;

    case VEB_CMD_SYNC_DUPLEX_CRYPTO:
        ret = veb_sym_duplex_crypto(priv->spi, &(data.cmd), buf, IV, data.inlen);
        break;

    default:
        ret = VEB_ERR_IOCTL;
        break;
    }
    mutex_unlock(&(priv->dev_lock));
    if (!!ret)
    {
        VEB_ERR("crpyto cmd %d fail \n", cmd);
        goto error;
    }
    if (data.type == VEB_DATA_COPY) {
      VEB_DDUMP_WITH_PREFIX("out  buf:", ((unsigned char *)buf), data.inlen);
      if (copy_to_user(data.outbuf, buf, data.outlen))
      {
          VEB_ERR(" copy_to_user error!\n");
          ret = VEB_ERR_COPY_TO_USER;
      }
    }

error:
    if (data.type == VEB_DATA_MMAP)
    {
        mutex_unlock(&(priv->mmap_lock));
    }
    else {
        kfree(buf);
    }

    return ret;
}
#if 0

int veb_compat_ioctl_crypto(veb_private_data_t *priv, unsigned int cmd, unsigned long arg)
{
    //unsigned char *IV = NULL, tmp[IV_LEN];
    int ret = VEB_OK;
    //veb_crypto_data_t data;
    //veb_crypto_com_data_t com_data;
    //compat_uint_t u;
    compat_uptr_t p;
    //unsigned char *tmp22;

    int err = 0;
    //char *buf = NULL;
    veb_crypto_com_data_t __user *datatest11; 
        
    //unsigned char* ptmp_buf = NULL;
    //compat_ptr((compat_uptr_t)data.iv)
    VEB_TRACE_IN();
    datatest11 = (veb_crypto_com_data_t __user *)arg;
    err = get_user(p, &(datatest11->inbuf));
     //tmp22 = (char*)p;
    VEB_ERR("p1 %d , err%d \n", p, err);
     if (err == 0) 
     {
       VEB_ERR("test11 err \n");
       VEB_DDUMP_WITH_PREFIX("p11 :", ((unsigned char *)compat_ptr(p)), sizeof(compat_uptr_t));
     }



    memset(&data, 0, sizeof(data));
    if(copy_from_user(&com_data, (void __user *)arg, sizeof(veb_crypto_com_data_t)))
    {
        VEB_ERR("copy from user failed!\n");
        return VEB_ERR_COPY_FROM_USER;
    }
    VEB_DDUMP_WITH_PREFIX("com_data :", ((unsigned char *)&com_data), sizeof(veb_crypto_com_data_t));
    memcpy(&data, &com_data, sizeof(cmd_t)+8);
    err = get_user(p, &(com_data.inbuf));
    //tmp22 = (char*)p;
    VEB_ERR("p %d , err%d \n", p, err);

    
    //tmp22 = (char*)com_data.inbuf;
    //VEB_ERR("p %d , \n", com_data.inbuf);
    if (err == 0) 
    {
      VEB_ERR("test err \n");
      VEB_DDUMP_WITH_PREFIX("p11 :", ((unsigned char *)compat_ptr(p)), sizeof(compat_uptr_t));
    }
    //VEB_DDUMP_WITH_PREFIX("p1 :", ((unsigned char *)&p), sizeof(compat_uptr_t));
    data.inbuf = compat_ptr(p);
    if (data.inbuf != NULL)
      VEB_ERR("data.inbuf %p  \n", data.inbuf);
    //err = put_user(compat_ptr(p), &(data.inbuf));
    err = get_user(u, &(com_data.outlen));
    com_data.outlen = u;
    //err = put_user(u, &(data.outlen));
    err = get_user(p, &(com_data.outbuf));
    VEB_DDUMP_WITH_PREFIX("p2 :", ((unsigned char *)&p), sizeof(compat_uptr_t));
    data.outbuf = compat_ptr(p);
    //err = put_user(compat_ptr(p), &(data.outbuf));
    err = get_user(p, &(com_data.iv));
    VEB_DDUMP_WITH_PREFIX("p3 :", ((unsigned char *)&p), sizeof(compat_uptr_t));
    data.iv = compat_ptr(p);
    //err = put_user(compat_ptr(p), &(data.iv));

    VEB_DDUMP_WITH_PREFIX("data :", ((unsigned char *)&data), sizeof(veb_crypto_data_t));
    if (data.iv != NULL)
    {
        IV = tmp;
        
        if(copy_from_user(IV, data.iv, IV_LEN))
        {
            VEB_ERR(" copy_from_user--- error!\n");
            return VEB_ERR_COPY_FROM_USER;
        }
    }

    if (data.type == VEB_DATA_MMAP)
    {
        mutex_lock(&(priv->mmap_lock));
        buf = priv->mmap_addr;
    }
    else if((data.type == VEB_DATA_COPY) && (data.inbuf != NULL))
    {
        int buf_len;

        buf_len = data.inlen + 128;
        if (VEB_CMD_SYNC_DUPLEX_CRYPTO == cmd)
            buf_len += 3 * XFER_BLOCK_SIZE;
        buf = kzalloc(buf_len, GFP_KERNEL);
        if (buf == NULL)
        {
            VEB_ERR("buf null nomen \n");
            return -ENOMEM;
        }
        VEB_ERR("inbuf= %ld!\n",sizeof(data.inbuf));
        if (copy_from_user(buf, data.inbuf, data.inlen))
        {
            kfree(buf);
            VEB_ERR(" copy_from_user+++ error!\n");
            return VEB_ERR_COPY_FROM_USER;
        }
    }
    else
    {
        VEB_ERR("type(%d) or s of struct veb_sdata is illegal!\n", data.type);
        return VEB_ERR_PARAM;
    }

    mutex_lock(&(priv->dev_lock));
    switch (cmd) 
    {
    case VEB_CMD_SYNC_CRYPTO:
        ret = veb_sym_crypto(priv->spi, &(data.cmd), buf, IV, data.inlen);
        break;

    case VEB_CMD_SYNC_DUPLEX_CRYPTO:
        ret = veb_sym_duplex_crypto(priv->spi, &(data.cmd), buf, IV, data.inlen);
        break;

    default:
        ret = VEB_ERR_IOCTL;
        break;
    }
    mutex_unlock(&(priv->dev_lock));
    if (!!ret)
    {
        VEB_ERR("crpyto cmd %d fail \n", cmd);
        goto error;
    }
    if (data.type == VEB_DATA_COPY) {
      if (copy_to_user(data.outbuf, buf, data.outlen))
      {
          VEB_ERR(" copy_to_user error!\n");
          ret = VEB_ERR_COPY_TO_USER;
      }
    }

error:
    if (data.type == VEB_DATA_MMAP)
    {
        mutex_unlock(&(priv->mmap_lock));
    }
    else {
        kfree(buf);
    }

    return ret;
}
#endif



int veb_ioctl_withkey_crypto(veb_private_data_t *priv, unsigned int cmd, unsigned long arg)
{
    unsigned char *IV = NULL, tmp[IV_LEN] = {0};
    int ret = VEB_OK;
    veb_withkey_data_t data;
    char *buf = NULL;

    VEB_TRACE_IN();
    if(copy_from_user(&data, (void __user *)arg, sizeof(veb_withkey_data_t)))
    {
        VEB_ERR("copy from user failed!\n");
        return VEB_ERR_COPY_FROM_USER;
    }

    if (data.iv != NULL)
    {
        IV = tmp;
        if(copy_from_user(IV, data.iv, IV_LEN))
        {
            VEB_ERR(" copy_from_user--- error!\n");
            return VEB_ERR_COPY_FROM_USER;
        }
    }

    if (data.type == VEB_DATA_MMAP)
    {
        mutex_lock(&(priv->mmap_lock));
        buf = priv->mmap_addr;
    }
    else if((data.type == VEB_DATA_COPY) && (data.inbuf != NULL))
    {
        buf = kzalloc(data.inlen + 128, GFP_KERNEL);
        if (buf == NULL)
        {
            VEB_ERR("buf null nomen \n");
            return -ENOMEM;
        }
        if (copy_from_user(buf, data.inbuf, data.inlen))
        {
            kfree(buf);
            VEB_ERR(" copy_from_user+++ error!\n");
            return VEB_ERR_COPY_FROM_USER;
        }
    }
    else
    {
        VEB_ERR("type(%d) or s of struct veb_sdata is illegal!\n", data.type);
        return VEB_ERR_PARAM;
    }

    mutex_lock(&(priv->dev_lock));
    ret = veb_sym_withkey_cmd_write(priv->spi, &(data.cmd), buf, &(data.veb_key),
              IV, data.inlen);
    mutex_unlock(&(priv->dev_lock));

    if (!!ret)
    {
        VEB_ERR("sym_withkey fail \n");
        goto error;
    }
    if (data.type == VEB_DATA_COPY) {
      if (copy_to_user(((veb_withkey_data_t *)arg)->outbuf, buf, data.outlen))
      {
          VEB_ERR(" copy_to_user error!\n");
          ret = VEB_ERR_COPY_TO_USER;
      }
    }
error:
    if (data.type == VEB_DATA_MMAP)
    {
        mutex_unlock(&(priv->mmap_lock));
    }
    else {
        kfree(buf);
    }

    return ret;
}

extern long veb_smartcard_ioctl(veb_private_data_t *, unsigned int cmd, unsigned long arg);
#ifdef CONFIG_SMARTCARD
extern long veb_smartcard_ioctl_read(veb_private_data_t *, unsigned int cmd, unsigned long arg);
extern long veb_smartcard_ioctl_exchange(veb_private_data_t *, unsigned int cmd, unsigned long arg);
extern long veb_smartcard_compat_ioctl_read(veb_private_data_t *, unsigned int cmd, unsigned long arg);
extern long veb_smartcard_compat_ioctl_exchange(veb_private_data_t *, unsigned int cmd, unsigned long arg);

#endif

static long veb_a5_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    a5_duplex_con a5_con;

    struct veb_data *data = NULL;
    veb_private_data_t *priv = NULL;
    struct miscdevice *miscdev = NULL;

    VEB_TRACE_IN();

    miscdev = (struct miscdevice *)(file->private_data);
    data = container_of(miscdev, struct veb_data, misc);
    priv = &(data->priv);
    // priv = (veb_private_data_t *)dev_get_drvdata(miscdev->this_device);

    if (_IOC_TYPE(cmd) != VEB_IOCTL_MAGIC)
    {
        VEB_ERR("This command(0x%08x) is not valid!\n", cmd);
        return -ENOTTY;
    }

    cmd = _IOC_NR(cmd);
    switch (cmd)
    {
    case A5_CHIP_CON_NEW:
        if(copy_from_user(&a5_con, (a5_duplex_con*)arg, sizeof(a5_duplex_con)))
        {
            VEB_ERR("copy from user failed!\n");
            return VEB_ERR_COPY_FROM_USER;
        }
        ret = veb_ioctl_duplex(priv, &a5_con);
        break;
    case VEB_CMD_CONFIG:
        ret = veb_ioctl_config(priv, cmd, arg);
        if(ret != VEB_OK)
        {
            VEB_ERR("veb_ioctl_config failed(%d)!\n", ret);
            return ret;
        }
        break;
    case VEB_CMD_READ:
        ret = veb_ioctl_read(priv, cmd, arg);
        if(ret != VEB_OK)
        {
            VEB_ERR("veb_a5_ioctl_read failed(%d)!\n", ret);
            return ret;
        }
        break;
    case VEB_CMD_WRITE:
        ret = veb_ioctl_write(priv, cmd, arg);
        if(ret != VEB_OK)
        {
            VEB_ERR("veb_a5_ioctl_write failed(%d)!\n", ret);
            return ret;
        }
        break;
    case VEB_CMD_EXCHANGE:
        ret = veb_ioctl_exchange(priv, cmd, arg);
        break;
    case VEB_CMD_RESTORE_MODE:
        ret = veb_ioctl_restore_mode(priv, cmd, arg);
        if(ret != VEB_OK)
        {
            VEB_ERR("veb_ioctl_restore_mode failed(%d)!\n", ret);
            return ret;
        }
        break;
    case VEB_CMD_UPGRADE:
        ret = veb_ioctl_upgrade(priv, cmd, arg);
        if(ret != VEB_OK)
        {
            VEB_ERR("veb_ioctl_upgrade failed(%d)!\n", ret);
            return ret;
        }
        break;

    case VEB_CMD_SYNC_CRYPTO:
    case VEB_CMD_SYNC_DUPLEX_CRYPTO:
        ret = veb_ioctl_crypto(priv, cmd, arg);
        break;
    case VEB_CMD_WITHKEY_SYNC_CRYPTO:
        ret = veb_ioctl_withkey_crypto(priv, cmd, arg);
        break;
    case VEB_CMD_SMARTCARD:
        ret = veb_smartcard_ioctl(priv, cmd, arg);
        break;
#ifdef CONFIG_SMARTCARD
    case VEB_CMD_SMARTCARD_READ:
        ret = veb_smartcard_ioctl_read(priv, cmd, arg);
        break;

    case VEB_CMD_SMARTCARD_EXCHANGE:
        ret = veb_smartcard_ioctl_exchange(priv, cmd, arg);
        break;
#endif
    case VEB_CMD_RESET:
        veb_spi_reset();
        ret = VEB_OK;
        break;
    default:
        VEB_ERR("command(0x%08x) is unknown\n", cmd);
        return VEB_ERR_PARAM;
    }

    VEB_TRACE_OUT();
    return ret;
}

static long veb_a5_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    a5_duplex_con a5_con;

    struct veb_data *data = NULL;
    veb_private_data_t *priv = NULL;
    struct miscdevice *miscdev = NULL;

    VEB_TRACE_IN();

    miscdev = (struct miscdevice *)(file->private_data);
    data = container_of(miscdev, struct veb_data, misc);
    priv = &(data->priv);
    // priv = (veb_private_data_t *)dev_get_drvdata(miscdev->this_device);

    if (_IOC_TYPE(cmd) != VEB_IOCTL_MAGIC)
    {
        VEB_ERR("This command(0x%08x) is not valid!\n", cmd);
        return -ENOTTY;
    }

    cmd = _IOC_NR(cmd);
    switch (cmd)
    {
    case A5_CHIP_CON_NEW:
        if(copy_from_user(&a5_con, (a5_duplex_con*)arg, sizeof(a5_duplex_con)))
        {
            VEB_ERR("copy from user failed!\n");
            return VEB_ERR_COPY_FROM_USER;
        }
        ret = veb_ioctl_duplex(priv, &a5_con);
        break;
    case VEB_CMD_CONFIG:
        ret = veb_ioctl_config(priv, cmd, arg);
        if(ret != VEB_OK)
        {
            VEB_ERR("veb_ioctl_config failed(%d)!\n", ret);
            return ret;
        }
        break;
    case VEB_CMD_READ:
        ret = veb_ioctl_read(priv, cmd, arg);
        if(ret != VEB_OK)
        {
            VEB_ERR("veb_a5_ioctl_read failed(%d)!\n", ret);
            return ret;
        }
        break;
    case VEB_CMD_WRITE:
        ret = veb_ioctl_write(priv, cmd, arg);
        if(ret != VEB_OK)
        {
            VEB_ERR("veb_a5_ioctl_write failed(%d)!\n", ret);
            return ret;
        }
        break;
    case VEB_CMD_EXCHANGE:
        ret = veb_ioctl_exchange(priv, cmd, arg);
        break;
    case VEB_CMD_RESTORE_MODE:
        ret = veb_ioctl_restore_mode(priv, cmd, arg);
        if(ret != VEB_OK)
        {
            VEB_ERR("veb_ioctl_restore_mode failed(%d)!\n", ret);
            return ret;
        }
        break;
    case VEB_CMD_UPGRADE:
        ret = veb_ioctl_upgrade(priv, cmd, arg);
        if(ret != VEB_OK)
        {
            VEB_ERR("veb_ioctl_upgrade failed(%d)!\n", ret);
            return ret;
        }
        break;

    case VEB_CMD_SYNC_CRYPTO:
    case VEB_CMD_SYNC_DUPLEX_CRYPTO:
        ret = veb_compat_ioctl_crypto(priv, cmd, arg);
        break;
    case VEB_CMD_WITHKEY_SYNC_CRYPTO:
        ret = veb_ioctl_withkey_crypto(priv, cmd, arg);
        break;
    case VEB_CMD_SMARTCARD:
        ret = veb_smartcard_ioctl(priv, cmd, arg);
        break;
#ifdef CONFIG_SMARTCARD
    case VEB_CMD_SMARTCARD_READ:
        ret = veb_smartcard_compat_ioctl_read(priv, cmd, arg);
        break;

    case VEB_CMD_SMARTCARD_EXCHANGE:
        ret = veb_smartcard_compat_ioctl_exchange(priv, cmd, arg);
        break;
#endif
    case VEB_CMD_RESET:
        veb_spi_reset();
        ret = VEB_OK;
        break;
    default:
        VEB_ERR("command(0x%08x) is unknown\n", cmd);
        return VEB_ERR_PARAM;
    }

    VEB_TRACE_OUT();
    return ret;
}




static int veb_a5_open(struct inode * inode, struct file *file)
{
    int ret = VEB_OK;

    VEB_TRACE_IN();

    ret = nonseekable_open(inode, file);
    if (ret)
        return ret;

    VEB_TRACE_OUT();
    return VEB_OK;
}

static ssize_t veb_a5_read(struct file *file, char __user *buf, size_t count, loff_t *f_pos)
{
    int ret = 0;
    unsigned char * rx_tmp = NULL;

    struct veb_data *data = NULL;
    veb_private_data_t *priv = NULL;
    struct miscdevice *miscdev = NULL;

    VEB_TRACE_IN();

    miscdev = (struct miscdevice *)(file->private_data);
    data = container_of(miscdev, struct veb_data, misc);
    priv = &(data->priv);
    // priv = (veb_private_data_t *)dev_get_drvdata(miscdev->this_device);
    
    rx_tmp = kmalloc(128*1024, GFP_KERNEL);
    if (rx_tmp == NULL) {
        VEB_ERR("buf malloc fail \n");
        return VEB_ERR_MALLOC;
    }
    mutex_lock(&(priv->dev_lock));
    veb_spi_wake_hardware(WAKE_TIME);
    /*
    ret = veb_wait_ready();
    if(0 != ret)
    {
        mutex_unlock(&(priv->dev_lock));
        VEB_ERR("Waiting for chipset failed(%d)!\n", ret);
        kfree(rx_tmp);
        return VEB_ERR_WAIT_BUSY_TIMEOUT;
    }
    */
    veb_spi_recv(priv->spi, rx_tmp, count);
    if (copy_to_user(buf,rx_tmp,count))
    {
        mutex_unlock(&(priv->dev_lock));
        VEB_ERR("copy to usert failed(%d)!\n", ret);
        kfree(rx_tmp);
        return VEB_ERR_COPY_TO_USER;
    }
    mutex_unlock(&(priv->dev_lock));
    
    kfree(rx_tmp);
    VEB_TRACE_OUT();
    return count;
}

static ssize_t veb_a5_write(struct file *file, const char __user *buf , size_t count, loff_t *f_pos)
{
    int ret = 0;
    unsigned char * tx_tmp = NULL;
    struct veb_data *data = NULL;
    veb_private_data_t *priv = NULL;
    struct miscdevice *miscdev = NULL;

    VEB_TRACE_IN();

    miscdev = (struct miscdevice *)(file->private_data);
    data = container_of(miscdev, struct veb_data, misc);
    priv = &(data->priv);
    // priv = (veb_private_data_t *)dev_get_drvdata(miscdev->this_device);
    tx_tmp = kmalloc(128*1024, GFP_KERNEL);
    if (tx_tmp == NULL) {
        VEB_ERR("buf malloc fail \n");
        return VEB_ERR_MALLOC;
    }
    
    mutex_lock(&(priv->dev_lock));
    if(copy_from_user(tx_tmp, (const char __user*)buf, count))
    {
        mutex_unlock(&(priv->dev_lock));
        VEB_ERR("copy from user failed(%d)!\n", ret);
        kfree(tx_tmp);
        return VEB_ERR_COPY_FROM_USER;
    }
    veb_spi_wake_hardware(WAKE_TIME);
    /*
    ret = veb_spi_wake(WAKE_TIME);
    if(0 != ret)
    {
        mutex_unlock(&(priv->dev_lock));
        VEB_ERR("Try to wake chipset up failed(%d)!\n", ret);
        kfree(tx_tmp);
        return ret;
    }
    */
    veb_spi_send(priv->spi, tx_tmp, count);
    mutex_unlock(&(priv->dev_lock));

    kfree(tx_tmp);
    VEB_TRACE_OUT();
    return count;
}

#if 0
static ssize_t veb_a5_read(struct file *file, char __user *buf, size_t count, loff_t *f_pos)
{
    int ret = 0;
    unsigned char * rx_tmp = NULL;
    veb_private_data_t *priv = NULL;
    struct miscdevice *miscdev = NULL;

    VEB_TRACE_IN();

    miscdev = (struct miscdevice *)(file->private_data);
    priv = (veb_private_data_t *)dev_get_drvdata(miscdev->this_device);

    mutex_lock(&(priv->dev_lock));
    ret = veb_wait_ready();
    if(0 != ret)
    {
        mutex_unlock(&(priv->dev_lock));
        VEB_ERR("Waiting for chipset failed(%d)!\n", ret);
        return VEB_ERR_WAIT_BUSY_TIMEOUT;
    }

    rx_tmp = (unsigned char*)priv->mmap_addr;
    veb_spi_recv(priv->spi, rx_tmp, count);
    mutex_unlock(&(priv->dev_lock));

    VEB_TRACE_OUT();
    return count;
}

static ssize_t veb_a5_write(struct file *file, const char __user *buf , size_t count, loff_t *f_pos)
{
    int ret = 0;
    unsigned char * tx_tmp = NULL;
    veb_private_data_t *priv = NULL;
    struct miscdevice *miscdev = NULL;

    VEB_TRACE_IN();

    miscdev = (struct miscdevice *)(file->private_data);
    priv = (veb_private_data_t *)dev_get_drvdata(miscdev->this_device);

    mutex_lock(&(priv->dev_lock));
    ret = veb_spi_wake(WAKE_TIME);
    if(0 != ret)
    {
        mutex_unlock(&(priv->dev_lock));
        VEB_ERR("Try to wake chipset up failed(%d)!\n", ret);
        return ret;
    }

    tx_tmp = (unsigned char*)priv->mmap_addr;
    veb_spi_send(priv->spi, tx_tmp, count);
    mutex_unlock(&(priv->dev_lock));

    VEB_TRACE_OUT();
    return count;
}
#endif

extern void veb_mtk_pin_init(void);
extern void veb_mtk_pin_suspend(void);
extern void veb_mtk_pin_resume(void);

#ifdef CFG_PLATFORM_MTK
#if 0
static int veb_a5_spi_resume(struct spi_device *spi)
{
    veb_private_data_t *priv = NULL;

    priv = (veb_private_data_t *)spi_get_drvdata(spi);
    veb_mtk_pin_init();
    mutex_unlock(&(priv->dev_lock));
    VEB_DBG("veb a5 resume ok\n");

    return 0;
}

static int veb_a5_spi_suspend(struct spi_device *spi, pm_message_t mesg)
{
    veb_private_data_t *priv = NULL;

    priv = (veb_private_data_t *)spi_get_drvdata(spi);
    if (!mutex_trylock(&(priv->dev_lock))){
        VEB_DBG("try veb a5 device lock failed!\n");
		return -EAGAIN;
    }
    veb_mtk_pin_suspend();

    VEB_DBG("veb a5 suspend(event %d) ok\n", mesg.event);
    return 0;
}
#endif

static int  veb_a5_spi_suspend(struct device *dev)
{
    veb_private_data_t *priv = NULL;
    priv = dev_get_drvdata(dev);
    if (!mutex_trylock(&(priv->dev_lock)))
    {
        VEB_DBG("try veb a5 device lock failed!\n");
		return -EAGAIN;
    }
    //veb_mtk_pin_suspend();
    VEB_DBG("veb a5 suspend ok\n");
    
    return 0;
}

static int  veb_a5_spi_resume(struct device *dev)
{
    veb_private_data_t *priv = NULL;
    priv = dev_get_drvdata(dev);
    //veb_mtk_pin_resume();
    mutex_unlock(&(priv->dev_lock));
    VEB_DBG("veb a5 resume ok\n");

    return 0;
}
const struct dev_pm_ops veb_a5_pm_ops = {
        .suspend = veb_a5_spi_suspend,
        .resume = veb_a5_spi_resume,
};

#else

static int veb_a5_spi_resume(struct device *dev)
{
    veb_private_data_t *priv = NULL;

    VEB_TRACE_IN();

    priv = dev_get_drvdata(dev);
    VEB_DBG("priv = %p\n", priv);
    mutex_unlock(&(priv->dev_lock));
    VEB_DBG("veb a5 resume ok\n");

    return 0;
}

static int veb_a5_spi_suspend(struct device *dev)
{
    veb_private_data_t *priv = NULL;

    VEB_TRACE_IN();
    priv = dev_get_drvdata(dev);
    if (!mutex_trylock(&(priv->dev_lock))){
        VEB_DBG("try veb a5 device lock failed!\n");
		return -EAGAIN;
    }
    VEB_TRACE_OUT();
    return 0;
}
#endif

#ifndef CFG_PLATFORM_MTK //qualcom
#ifdef CONFIG_PM_SLEEP
const struct dev_pm_ops veb_pm_ops = {
    .suspend    = veb_a5_spi_suspend,
    .resume     = veb_a5_spi_resume,
};
#endif
#endif

static int veb_a5_mmap(struct file *file, struct vm_area_struct *vma)
{
    int ret = VEB_OK;
    unsigned long phys;

    struct veb_data *data = NULL;
    veb_private_data_t *priv = NULL;
    struct miscdevice *miscdev = NULL;

    VEB_TRACE_IN();
    miscdev = (struct miscdevice *)(file->private_data);
    data = container_of(miscdev, struct veb_data, misc);
    priv = &(data->priv);
    // priv = (veb_private_data_t *)dev_get_drvdata(miscdev->this_device);

    mutex_lock(&(priv->dev_lock));
    phys = virt_to_phys(priv->mmap_addr);
    ret = remap_pfn_range(vma,
                          vma->vm_start,
                          phys >> PAGE_SHIFT,
                          vma->vm_end - vma->vm_start,
                          vma->vm_page_prot);
    mutex_unlock(&(priv->dev_lock));

    if(ret != 0)
    {
        VEB_ERR("remap_pfn_range failed(%d)!\n", ret);
        return ret;
    }

    VEB_TRACE_OUT();
    return 0;
}

static const struct file_operations veb_a5_fops = {
    .owner              = THIS_MODULE,
    .open               = veb_a5_open,
    .read               = veb_a5_read,
    .write              = veb_a5_write,
    .unlocked_ioctl     = veb_a5_ioctl,
    .compat_ioctl     = veb_a5_compat_ioctl,
    .mmap               = veb_a5_mmap,
};

static int veb_a5_spi_probe(struct spi_device *spi)
{
    int ret = 0;
    int n = 0;
    veb_data_t *data = NULL;
    veb_private_data_t *priv = NULL;
	printk("################veb_a5_spi_probe#########################");
    VEB_TRACE_IN();
    ret = veb_platform_init(spi);
	printk("!!!!!!!!!!!!!!!!!!!!!!!!veb_a5_spi_probe#########################");
    if(ret != VEB_OK)
    {
        VEB_ERR("veb initialize in platform failed(%d)!\n", ret);
        return ret;
    }

    data = kzalloc(sizeof(struct veb_data), GFP_KERNEL);
    if (NULL == data)
    {
        VEB_ERR("kzalloc for veb data failed!\n");
        return -ENOMEM;
    }

    memset(data, 0, sizeof(struct veb_data));
    priv = &(data->priv);

    data->misc.minor = MISC_DYNAMIC_MINOR;
    data->misc.name = &(veb_a5_name[0]);
    data->misc.fops = &veb_a5_fops;

    /**
     * Initialize veb private data
     */
    priv->spi = spi;
    priv->miscdev = &(data->misc);
    mutex_init(&(priv->dev_lock));
    mutex_init(&(priv->mmap_lock));
    priv->mmap_addr = (void *)__get_free_pages(GFP_KERNEL, get_order(VEB_MMAP_SIZE));
    VEB_DBG("order:PAGE_SIZE = %d:%ld\n", get_order(VEB_MMAP_SIZE), PAGE_SIZE);
    if(priv->mmap_addr != NULL){
        for(n = 0; n < VEB_MMAP_SIZE / PAGE_SIZE; n++)
        {
            SetPageReserved(virt_to_page(priv->mmap_addr + n * PAGE_SIZE));
        }
    }else{
        veb_platform_exit(spi);
        kfree(priv);
        VEB_ERR("get free pages(size:%d) failed!\n", VEB_MMAP_SIZE);
        return -ENOMEM;
    }

    /**
     * Store veb private data in spi driver_data
     */
    spi_set_drvdata(spi, priv);

    /**
     * Initialize spi configure
     */
#ifdef CFG_PLATFORM_MTK
    spi->mode = VEB_SPI_MODE;
#else
    spi->mode = spi->mode|SPI_MODE_3|SPI_CS_HIGH;
#endif
    spi->bits_per_word = VEB_SPI_BITS_PER_WORD;

#ifdef CFG_PLATFORM_MTK
    //ret = spi_setup(spi);
    if(spi->dev.of_node){
		irq_gpio = of_get_named_gpio(spi->dev.of_node, "veba5,irq-gpio", 0);
		if(irq_gpio > 0){
			VEB_ERR("veb gpio_irq[%d] \n", gpio_to_irq(irq_gpio));
		}
		gpio_direction_input(irq_gpio);	
	}
    if (ret < 0) {
        veb_platform_exit(spi);
        free_pages((unsigned long)priv->mmap_addr, get_order(VEB_MMAP_SIZE));
        kfree(priv);
        VEB_ERR("veb_a5 spi spi_setup error!\n");
        return ret;
    }
#endif
    /**
     * register miscdevice and store veb private data in misc driver_data
     */
    ret = misc_register(&(data->misc));
    if (ret < 0)
    {
        veb_platform_exit(spi);
        free_pages((unsigned long)priv->mmap_addr, get_order(VEB_MMAP_SIZE));
        kfree(priv);
        VEB_ERR("register misc(%s) device failed(%d)\n", VEB_A5_NAME, ret);
        return ret;
    }
    // dev_set_drvdata(veb_a5_miscdev.this_device, priv);
    //veb_spi_reset();

    VEB_INF("%s: v%s\n", VEB_A5_DESC, VEB_A5_VERSION);
    VEB_TRACE_OUT();
    return ret;
}

static int veb_a5_spi_remove(struct spi_device *spi)
{
    veb_private_data_t *priv = NULL;

    priv = spi_get_drvdata(spi);
    free_pages((unsigned long)priv->mmap_addr, get_order(VEB_MMAP_SIZE));
    kfree(priv);
    priv = NULL;

    return veb_platform_exit(spi);
}

#ifdef CFG_VEB_OF
static struct of_device_id veb_a5_spi_table[] = {
	{.compatible = "veb,veba5-spi",},
    {.compatible = "zyt,veba5-spi",},
    {.compatible = "zyt,veba3-spi",},
    { },
};
#endif

struct spi_device_id spi_id_table_veb_a5 = {"veb_a5_spi", 0};
#ifdef CFG_PLATFORM_MTK
#if 0
static struct spi_board_info spi_board_devs[] __initdata = {
    [0] = {
        .modalias       = "veb_a5_spi",
        .bus_num        = VEB_SPI_BUS_NUM,
        .chip_select    = VEB_SPI_CHIP_SELECT,
        .mode           = VEB_SPI_MODE,
    },
};
#endif
#endif

static struct spi_driver veb_a5_spi_driver = {
    .driver = {
        .name        = "veb_spi_veba5",
        .owner        = THIS_MODULE,
        .bus = &spi_bus_type,
#ifdef CFG_VEB_OF
        .of_match_table = veb_a5_spi_table,
#endif

#ifndef CFG_PLATFORM_MTK //qualcom
        .pm	= &veb_pm_ops,
#else
        .pm	= &veb_a5_pm_ops,
#endif
    },

    .probe      = veb_a5_spi_probe,
    .remove     = veb_a5_spi_remove,
#ifdef CFG_PLATFORM_MTK
    
    //.suspend    = veb_a5_spi_suspend,
    //.resume     = veb_a5_spi_resume,
#endif
    .id_table   = &spi_id_table_veb_a5,
};

static int __init veb_a5_spi_init(void)
{
    int rc = 0;

    VEB_TRACE_IN();
#ifdef CFG_PLATFORM_MTK
    //spi_register_board_info(spi_board_devs, ARRAY_SIZE(spi_board_devs));
#endif
    rc = spi_register_driver(&veb_a5_spi_driver);
    if (rc < 0)
    {
        VEB_ERR("vebv3_spi register fail : %d\n", rc);
        return rc ;
    }

    VEB_TRACE_OUT();
    return rc ;
}

static void __exit veb_a5_spi_exit(void)
{
    spi_unregister_driver(&veb_a5_spi_driver);

    return;
}

module_init(veb_a5_spi_init);
module_exit(veb_a5_spi_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ZYT Software");
MODULE_DESCRIPTION("veb_a5 spi driver ");
MODULE_VERSION("1.0");
