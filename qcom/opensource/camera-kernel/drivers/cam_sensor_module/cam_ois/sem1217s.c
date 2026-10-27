#include <linux/module.h>
#include <linux/firmware.h>
#include <linux/slab.h>
#include <cam_sensor_cmn_header.h>
#include "cam_ois_core.h"
#include "cam_ois_soc.h"
#include "cam_sensor_util.h"
#include "cam_debug_util.h"
#include "cam_res_mgr_api.h"
#include "cam_common_util.h"
#include "cam_packet_util.h"
#include "sem1217s.h"

int32_t i2c_read_data(struct cam_ois_ctrl_t *o_ctrl, uint32_t addr, uint32_t length, uint8_t *data)
{
	int32_t rc = 0;
	int32_t i = 0;

	rc = camera_io_dev_read_seq(&o_ctrl->io_master_info, addr, data,
				    CAMERA_SENSOR_I2C_TYPE_WORD, CAMERA_SENSOR_I2C_TYPE_BYTE,
				    length);

	for (i = 0; i < length; i++) {
		CAM_DBG(CAM_OIS, "[SEM1217S] read addr 0x%04x[%d] = 0x%02x", addr, i, data[i]);
	}

	if (rc) {
		CAM_ERR(CAM_OIS, "[SEM1217S] Failed to read, rc: %d", rc);
	}

	return rc;
}

int32_t i2c_write_data(struct cam_ois_ctrl_t *o_ctrl, uint32_t addr, uint32_t length, uint8_t *data,
		       uint32_t delay)
{
	struct cam_sensor_i2c_reg_array *w_data;
	struct cam_sensor_i2c_reg_setting write_setting;
	uint32_t i = 0;
	int32_t rc = 0;

	if (!data || !o_ctrl || !length || length > 256) {
		CAM_ERR(CAM_OIS, "[SEM1217S] Invalid Args");
		return -EINVAL;
	}

	w_data = kcalloc(length, sizeof(*w_data), GFP_KERNEL);
	if (!w_data)
		return -ENOMEM;

	for (i = 0; i < length; i++) {
		w_data[i].reg_addr = addr;
		w_data[i].reg_data = data[i];
		w_data[i].delay = 0;
		w_data[i].data_mask = 0;
	}

	write_setting.size = length;
	write_setting.addr_type = CAMERA_SENSOR_I2C_TYPE_WORD;
	write_setting.data_type = CAMERA_SENSOR_I2C_TYPE_BYTE;
	write_setting.delay = delay;
	write_setting.reg_setting = w_data;

	rc = camera_io_dev_write_continuous(&(o_ctrl->io_master_info), &write_setting,
					    CAM_SENSOR_I2C_WRITE_SEQ);

	if (rc < 0) {
		CAM_ERR(CAM_OIS, "[SEM1217S] OIS i2c_write_data write failed, rc: %d", rc);
	}

	kfree(w_data);
	return rc;
}

int32_t load_fw_buff(struct cam_ois_ctrl_t *o_ctrl, char *firmware_name, uint8_t *read_data,
		     uint32_t read_length)
{
	const struct firmware *fw;
	int rc;

	if (!o_ctrl || !firmware_name || !read_data || !read_length)
		return -EINVAL;

	rc = request_firmware(&fw, firmware_name, o_ctrl->soc_info.dev);
	if (rc)
		return rc;

	if (fw->size < read_length)
		rc = -EINVAL;
	else
		memcpy(read_data, fw->data, read_length);

	release_firmware(fw);
	return rc;
}
