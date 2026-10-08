GInputVC 1.11
by Silent


DESCRIPTION

	Hey, what's going on? You plugged in your PlayStation or XBOX pad in hopes that you'll be able to
	play GTA on your PC just like you used to play on the console, and for some reason you can't make
	the controls work properly? You can't use full possibilities of your brand new, XInput-compatible
	pad? Maybe you want to refresh your childhood memories after ages of playing IV and you're sick
	of the old-fashioned controls?
	If any of these apply to you, GInput is the mod you've been looking for! This modification
	completely rewrites Vice City controls handling and ditches DirectInput in favour of XInput. This
	way, your PlayStation 3 and XBOX 360 pads will be handled by the game just perfectly, taking
	advantage of all their features, including analog triggers. If used with DualShock 3 controller and SCP
	Driver Package, it can go as far as making full use of SIXAXIS feature, just like some PS3 titles do!

	This modification features:

	* Complete XInput support, so pads are mapped to match console versions perfectly (including Start
	  button, which can't be mapped on PC version without this modification).
	* Proper vibration support - something that was cut from all PC versions of the game!
	* FIVE different control mappings - four setups matching PS2 selectable setups and fifth one
	  being a recreation of GTA IV controls!
	* Pad buttons in In-Game helps, replacing PC key names (depending on user's choice, either
	  PlayStation or XBOX buttons are shown).
	* An INI file with tons of options - including toggleable vibration, selecting controls setup,
	  "Invert Look" option from console versions, axis inversion options from San Andreas and more!
	* Automatic switching between keyboard & mouse and pad controls, basing on last device input.
	* Support for Guide/PS button. The button is fully functional and can show your Steam Overlay
	  menu if you launched the game via Steam!
	* Cheats input from the pad. Use classic PS2 cheats on PC!
	* Increased support for DualShock 3 controllers and SCP Driver Package - including pressure sensitive
	  buttons and SIXAXIS accelerometers

	For complete controls list, see docs/GAME CONTROLS FULL LIST.txt.


INSTALLATION

	Easy as pie. Put 'GinputVC.asi' and 'GinputVC.ini' files in the game root directory or
	'mss' folder, then put both TXD files in 'models' folder. You can use the INI file to
	configure some mod properties, make sure you check it before you try out the mod!

	If your pad doesn't support XInput, you should try downloading any X360 controller emulator
	(x360ce recommended) and put it in the game folder (GInput makes use of xinput1_3.dll).

	If you are using a DualShock 3 controller and SCP Driver Package, you can put SCP's xinput1_3.dll (1.2.2.175 REQUIRED)
	in your game directory. This will allow GInput to make use of DS3's presusre sensitive button and SIXAXIS accelerometers!

	GInput comes with official, alternative button textures. They can be downloaded here:
	http://www.gtagarage.com/mods/show.php?id=24272

	Make sure you toggle 'Classic Controls' in game too!


USAGE

	The changes take effect automatically, no need to toggle anything when the game is launched.


SUPPORTED GAME VERSIONS

	* GTA VC 1.0 (all versions)
	* GTA VC 1.1 (all versions, including Steam version)


CHANGELOG

	* 1.11 (GTA III/GTA VC/GTA SA)
	  - Fixed a GInputSA issue where cheat input wouldn't work properly with 2 pads connected
	  - Fixed a GInputSA issue where SNEAK_ABOUT key wasn't displayed properly
	  - Fixed a GInputSA issue in IV controls mode where aiming with a melee weapon and switching to a previous target
	    (with LB/L1) would display vital stats
	  - New text override for GTA SA Burglary mission help text, matching the text displayed on PS2
	  - Introduced a new INI option to globally enable/disable SIXAXIS override hack - now needs to be explicitly
	    enabled in order to use any SIXAXIS features
	  - SIXAXIS override flag is now applied to hydraulics controls too
	  - In addition to existing 0-100 analog stick sensitivity, 101 - 200 values can now be used too to achieve
	    sensitivity higher than linear
	  - Fixed a GInputIII/GInputVC issue where buttons wouldn't show in texts formatted in a specific way - most notably,
	    occuring often when using non-English languages
	  - Minor code efficiency improvements

	* 1.10a (GTA III/GTA VC/GTA SA)
	  - Fixed a crash when calling out Steam Overlay

	* 1.10 (GTA III/GTA VC/GTA SA)
	  - First release of GInputSA
	  - Added SCP pressure sensitive button support - if using a DualShock 3 pad, GInput can make full use of SCP's xinput1_3.dll
	  - Added SIXAXIS accelerometers support - requires SCP Driver Package xinput1_3.dll
	  - Implemented GXT overrides which allow to correct all misleading helps - now they should all show correct buttons
	  - Added more button textures - now the game can use generic analog stick textures and dpad buttons with multiple
	    directions lit at once
	  - Implemented variable-width textures - now buttons maintain their aspect ratios when displayed
	  - Fixed a GInputIII/GInputVC issue where the camera wouldn't be switchable with ControlsSet=2
	  - Fixed a GInputIII/GInputVC issue where Southpaw option would break some keyboard controlls

	* 1.03 (GTA III/GTA VC)
	  - Fixed an issue in GInputVC where displaying pad buttons caused the game to go buggy
		(such as left analog stick locking)
	  - Fixed a GInputIII issue where pressing Start in menu prior to loading a save or starting a new game
		would result in a crash
	  - The game will not crash if xinput1_3.dll is not present in the system now - instead, GInput
		will show an error dialog and then unload
	  - The game will not crash if GInput is installed incorrectly - instead, if pad button textures
		can't be loaded, GInput will show an error and then shut down the game
	  - Improved input detection (for switching between keyboard and pad buttons in helps)
	  - Button drawing code has been rewritten from scratch, improving the way buttons are shown.
		Most notably, they now have their own width so they will not stick out of help boxes
	  - Southpaw option has been added
	  - Changed the way game handles looking around in IV controls by default - now, you look left/right
	    by pressing L1/LB and R1/RB. The old way of looking around with the right analog stick can be restored
		by setting DrivebyWithAnalog to 1.
	  - Menu controls have been unified between III and VC - now, both games use Start button
		the same way, and both games use Triangle to go back to the previous menu
	  - In addition, when using IV controls mode, Circle is used to go back to the previous menu instead
		of Triangle
	  - VC Stats and Map menu controls reworked - analog stick controls are now more precise and zooming
		the map in/out with triggers is now pressure sensitive
	  - Fixed several bugs related to radio scrolling in IV controls mode
	  - Some NEW controls have been added to the game:
		1) Holding Change Radio Station button when MP3 Player is selected will now cycle through MP3
		   tracks (works both from the pad and the keyboard)
		2) Pressing Toggle Submission/R3 button when driving a car with hydraulics will make it jump,
		   similarly to San Andreas (works both from the pad and the keyboard)
		3) Hold Change Camera button on a pad will now toggle a replay. Press Change Camera again to go
		   back in game
	  - Mod's INI file now can be modified on the fly - it will be reloaded after going back into the game
		from the minimised stats
	  - Pad input will not be registered now if the game is minimised
	  - Cheats now can be activated from the pad using PS2 key combinations. For the full list, see
		docs/cheat_list_iii_*.html

	* 1.02 (GTA III/GTA VC)
	  - First release of GInputIII
	  - Modification is now compatible with XP SP1 (which is now the oldest supported system,
	    1.0 and 1.01 required XP SP2 or newer)
	  - In IV controls mode, detonator is now controlled by Melee Attack (Circle/B) key
	  - Triggers handling reworked (deadzone is now smoother, making controls more precise)
	  - Triggers are now properly pressure sensitive when rotating the helicopter
	  - Rumble improved a bit (it should stop shaking in the same situations PS2 version did)
	  - Mouse move doesn't toggle 'keyboard + mouse mode' on anymore - now only button presses do
	  - INI file is now read correctly when inside 'mss' folder
	  - GInput now features a small API for modders (see 'GInputAPI' directory) which can be used
	    to check if pad is plugged in/is in use

	* 1.01 (GTA VC)
	  - Fixed a crash when using Steam EXE which occured when pickup collection help box was
	    about to appear

	* 1.0 (GTA VC)
	  - First release of GInputVC


CREDITS

	Deji - original GInput for San Andreas - huge inspiration for myself!
	Ash_735 - testing, complete art support
	Bugbear Entertainment - Steering Sensitivity option


CONTACT

	zdanio95@gmail.com - e-mail
	zdanio95 - Skype

Subscribe to my YouTube channel for more footage from my mods!
https://www.youtube.com/user/CookiePLMonster

Follow my Twitter account to be up to all my mods updates!
http://twitter.com/__silent_