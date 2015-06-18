/*
  This describes the hardware to the kernel and userspace software.
  A pointer to this table is stored at address 0x100.

  TODO: Flesh out the format of this. Add a version number. Include
  more info!
 */
unsigned int hw_desc_table[] = {
  CONFIG_GRLCD_FLIP,
};
