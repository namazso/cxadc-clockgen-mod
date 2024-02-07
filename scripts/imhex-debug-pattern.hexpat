// https://github.com/WerWolv/ImHex/wiki/Pattern-Language-Guide#endian-specification

struct debug_info {
	// should be 0x11223344
	le u32 magic;

	// true if the general startup of the si5351 clock generator chip looks fine
	u8 si5351_init_success;
	
	// true if there was activity on the PCM1802 lines, updated occasionally
	u8   pcm1802_activity_lrck;
	u8   pcm1802_activity_bck;
	u8   pcm1802_activity_data;

	// some specific counters on the PCM1802 subsystem
	le u32  pcm1802_out_of_sync_drops;
	le u32  pcm1802_rch_tmo_count;
	le u32  pcm1802_rch_tmo_value;

	// Counts RX timeout conditions in main1
	le u32 main1_rxsample_tmo;
};
