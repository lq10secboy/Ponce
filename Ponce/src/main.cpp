//! \file
/*
**  Copyright (c) 2016 - Ponce
**  Authors:
**         Alberto Garcia Illera        agarciaillera@gmail.com
**         Francisco Oca                francisco.oca.gonzalez@gmail.com
**
**  This program is under the terms of the BSD License.
*/

//IDA
#include <idp.hpp>
#include <dbg.hpp>
#include <loader.hpp>
#include <kernwin.hpp>

//Ponce
#include "callbacks.hpp"
#include "actions.hpp"
#include "globals.hpp"
#include "trigger.hpp"
#include "context.hpp"
#include "utils.hpp"
#include "formConfiguration.hpp"

//Triton
#include <api.hpp>

/*This function is called once in the IDA plugin init event to set the static configuration for triton. Architecture and memory/registry callbacks.*/
void triton_init()
{
	//We need to set the architecture for Triton
	triton::api.setArchitecture(TRITON_ARCH);
	// Memory access callback
	triton::api.addCallback(needConcreteMemoryValue);
	// Register access callback
	triton::api.addCallback(needConcreteRegisterValue);
	// This optimization is veeery good for the size of the formulas
	triton::api.enableSymbolicOptimization(triton::engines::symbolic::ALIGNED_MEMORY, true);
	// We only are symbolic or taint executing an instruction if it is tainted, so it is a bit faster and we save a lot of memory
	triton::api.enableSymbolicOptimization(triton::engines::symbolic::ONLY_ON_SYMBOLIZED, true);
	triton::api.enableSymbolicOptimization(triton::engines::symbolic::ONLY_ON_TAINTED, true);
}

//--------------------------------------------------------------------------
void idaapi run(int)
{
	/*We shouldn't prompt for it if the user has a saved configuration*/
	if (!load_options(&cmdOptions))
	{
		prompt_conf_window();
	}

	if (!hooked)
	{
		//Registering action for the Ponce config
		register_action(action_IDA_show_config);
		attach_action_to_menu("Edit/Ponce/", "Ponce:show_config", SETMENU_APP);
		//Registering action for the Ponce taint window
		register_action(action_IDA_show_taintWindow);
		attach_action_to_menu("Edit/Ponce/", "Ponce:show_taintWindows", SETMENU_APP);

		//First we ask the user to take a snapshot, -1 is to cancel so we don't run the plugin
		if (ask_for_a_snapshot() != -1)
		{
			if (!hook_to_notification_point(HT_UI, ui_callback, NULL))
			{
				warning("Could not hook ui callback");
				return;
			}
			if (!hook_to_notification_point(HT_DBG, tracer_callback, NULL))
			{
				warning("Could not hook tracer callback");
				return;
			}
		
			triton_init();
			msg("[+] Ponce plugin running!\n");
			hooked = true;
		}
	}
}

//--------------------------------------------------------------------------
int idaapi init(void)
{
	for (int i = 0;; i++)
	{
		if (action_list[i].action_decs == NULL){
			break;
		}
		//Here we register all the actions
		if (!register_action(*action_list[i].action_decs))
		{
			warning("Failed to register %s actions. Exiting Ponce plugin\n", action_list[i].action_decs->name);
			return PLUGIN_SKIP;
		}	
	}
	//We want to autorun the plugin when IDA starts?
	if (AUTO_RUN)
		run(0);
	return PLUGIN_KEEP;
}

//--------------------------------------------------------------------------
void idaapi term(void)
{
	// just to be safe
	unhook_from_notification_point(HT_UI, ui_callback, NULL);
	unhook_from_notification_point(HT_DBG, tracer_callback, NULL);
	hooked = false;
}

//--------------------------------------------------------------------------
//
//      PLUGIN DESCRIPTION BLOCK
//
//--------------------------------------------------------------------------
plugin_t PLUGIN =
{
	IDP_INTERFACE_VERSION,
	0,                    // plugin flags
	init,                 // initialize
	term,                 // terminate. this pointer may be NULL.
	run,                  // invoke plugin
	"Ponce, a concolic execution plugin for IDA", // long comment about the plugin
	"", // multiline help about the plugin
	"Ponce", // the preferred short name of the plugin
	"Ctrl+Shift+Z" // the preferred hotkey to run the plugin
};