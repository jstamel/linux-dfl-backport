// SPDX-License-Identifier: GPL-2.0

/* Intel(R) Memory based QSFP driver For DFL based devices.
 *
 * Copyright (C) 2022 Intel Corporation. All rights reserved.
 */
#include <linux/dfl.h>
#include <linux/phy/qsfp-mem.h>
#include <linux/module.h>

ssize_t dfl_qsfp_connected_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct qsfp *qsfp = dev_get_drvdata(dev);
	u32 plugin = qsfp_connected_show(qsfp);

	return sysfs_emit(buf, "%u\n", plugin);
}

static DEVICE_ATTR_RO(dfl_qsfp_connected);

static struct attribute *qsfp_mem_attrs[] = {
	&dev_attr_dfl_qsfp_connected.attr,
	NULL,
};
ATTRIBUTE_GROUPS(qsfp_mem);

static int qsfp_dfl_probe(struct dfl_device *dfl_dev)
{
	struct device *dev = &dfl_dev->dev;
	struct qsfp *qsfp;
	int ret = 0;

	qsfp = devm_kzalloc(dev, sizeof(*qsfp), GFP_KERNEL);
	if (!qsfp)
		return -ENOMEM;

	qsfp->base = devm_ioremap_resource(dev, &dfl_dev->mmio_res);
	if (!qsfp->base)
		return -ENOMEM;

	qsfp->dev = dev;
	mutex_init(&qsfp->lock);

	dev_set_drvdata(dev, qsfp);

	ret = qsfp_init_work(qsfp);
	if (ret != 0) {
		dev_err(dev, "Failed to initialize delayed work to read QSFP\n");
		return ret;
	}

	ret = qsfp_register_regmap(qsfp);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0) && RHEL_RELEASE_CODE < 0x803
	if (ret)
		return ret;

	ret = device_add_groups(dev, qsfp_mem_groups);
#endif
	return ret;
}

static void qsfp_dfl_remove(struct dfl_device *dfl_dev)
{
	struct device *dev = &dfl_dev->dev;
	struct qsfp *qsfp = dev_get_drvdata(dev);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0) && RHEL_RELEASE_CODE < 0x803
	device_remove_groups(dev, qsfp_mem_groups);
#endif
	qsfp_remove_device(qsfp);
}

#define FME_FEATURE_ID_QSFP 0x13

static const struct dfl_device_id qsfp_ids[] = {
	{ FME_ID, FME_FEATURE_ID_QSFP },
	{ }
};

static struct dfl_driver qsfp_driver = {
	.drv = {
		.name = "qsfp-mem",
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0) || RHEL_RELEASE_CODE >= 0x803
		.dev_groups = qsfp_mem_groups,
#endif
	},
	.id_table = qsfp_ids,
	.probe = qsfp_dfl_probe,
	.remove = qsfp_dfl_remove,
};

module_dfl_driver(qsfp_driver);
MODULE_ALIAS("dfl:t0000f0013");
MODULE_DEVICE_TABLE(dfl, qsfp_ids);
MODULE_DESCRIPTION("Intel(R) Memory based QSFP DFL driver");
MODULE_AUTHOR("Intel Corporation");
MODULE_LICENSE("GPL");
