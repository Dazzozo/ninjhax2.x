ninjhax
=======

list of files to build :
 
	cn_qr_initial_loader
		qr code that ROPs its way to gspwn, gets code exec, downloads cn_secondary_payload through HTTP and launches it
	cn_save_initial_loader
		modified savegame file that loads cn_secondary_payload from save and launches it
 
	cn_secondary_payload
		finishes cn cleanup, takes over spider with spider_initial_rop, then waits for it to return, sets up bootloader through HB command and uses it to launch hb_menu
 
	spider_initial_rop
		takes over spider thread0
	spider_thread0_rop
		takes over ro and jumps to code
 
	ro_initial_rop
		gets code exec in ro and jumps to code
	ro_initial_code
		reprotects spider mem and returns to spider
	ro_command_handler
		handles HB service commands
 
	spider_code
		does spider cleanup, returns handles to cn
 
	cn_bootloader
		calls HB command to load homebrew, then flushes/invalidates cache and jumps to app code
 
	installer
		installs the exploit to the savegame

	hb_menu
		lists homebrew apps on SD and uses bootloader to start them
