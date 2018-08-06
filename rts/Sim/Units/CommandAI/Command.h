/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <climits> // INT_MAX
#include <cstring> // memset

#include "System/creg/creg_cond.h"
#include "System/float3.h"



// ID's lower than 0 are reserved for build options (cmd -x = unitdefs[x])
#define CMD_STOP                   0
#define CMD_INSERT                 1
#define CMD_REMOVE                 2
#define CMD_WAIT                   5
#define CMD_TIMEWAIT               6
#define CMD_DEATHWAIT              7
#define CMD_SQUADWAIT              8
#define CMD_GATHERWAIT             9
#define CMD_MOVE                  10
#define CMD_PATROL                15
#define CMD_FIGHT                 16
#define CMD_ATTACK                20
#define CMD_AREA_ATTACK           21
#define CMD_GUARD                 25
#define CMD_AISELECT              30 //FIXME REMOVE
#define CMD_GROUPSELECT           35
#define CMD_GROUPADD              36
#define CMD_GROUPCLEAR            37
#define CMD_REPAIR                40
#define CMD_FIRE_STATE            45
#define CMD_MOVE_STATE            50
#define CMD_SETBASE               55
#define CMD_INTERNAL              60
#define CMD_SELFD                 65
#define CMD_SET_WANTED_MAX_SPEED  70
#define CMD_LOAD_UNITS            75
#define CMD_LOAD_ONTO             76
#define CMD_UNLOAD_UNITS          80
#define CMD_UNLOAD_UNIT           81
#define CMD_ONOFF                 85
#define CMD_RECLAIM               90
#define CMD_CLOAK                 95
#define CMD_STOCKPILE            100
#define CMD_MANUALFIRE           105
#define CMD_RESTORE              110
#define CMD_REPEAT               115
#define CMD_TRAJECTORY           120
#define CMD_RESURRECT            125
#define CMD_CAPTURE              130
#define CMD_AUTOREPAIRLEVEL      135
#define CMD_IDLEMODE             145
#define CMD_FAILED               150

#define CMDTYPE_ICON                        0  // expect 0 parameters in return
#define CMDTYPE_ICON_MODE                   5  // expect 1 parameter in return (number selected mode)
#define CMDTYPE_ICON_MAP                   10  // expect 3 parameters in return (mappos)
#define CMDTYPE_ICON_AREA                  11  // expect 4 parameters in return (mappos+radius)
#define CMDTYPE_ICON_UNIT                  12  // expect 1 parameters in return (unitid)
#define CMDTYPE_ICON_UNIT_OR_MAP           13  // expect 1 parameters in return (unitid) or 3 parameters in return (mappos)
#define CMDTYPE_ICON_FRONT                 14  // expect 3 or 6 parameters in return (middle of front and right side of front if a front was defined)
#define CMDTYPE_ICON_UNIT_OR_AREA          16  // expect 1 parameter in return (unitid) or 4 parameters in return (mappos+radius)
#define CMDTYPE_NEXT                       17  // used with CMD_INTERNAL
#define CMDTYPE_PREV                       18  // used with CMD_INTERNAL
#define CMDTYPE_ICON_UNIT_FEATURE_OR_AREA  19  // expect 1 parameter in return (unitid or featureid+unitHandler.MaxUnits() (id>unitHandler.MaxUnits()=feature)) or 4 parameters in return (mappos+radius)
#define CMDTYPE_ICON_BUILDING              20  // expect 3 parameters in return (mappos)
#define CMDTYPE_CUSTOM                     21  // used with CMD_INTERNAL
#define CMDTYPE_ICON_UNIT_OR_RECTANGLE     22  // expect 1 parameter in return (unitid)
                                               //     or 3 parameters in return (mappos)
                                               //     or 6 parameters in return (startpos+endpos)
#define CMDTYPE_NUMBER                     23  // expect 1 parameter in return (number)


// wait codes
#define CMD_WAITCODE_TIMEWAIT    1.0f
#define CMD_WAITCODE_DEATHWAIT   2.0f
#define CMD_WAITCODE_SQUADWAIT   3.0f
#define CMD_WAITCODE_GATHERWAIT  4.0f


// bits for the option field of Command
// NOTE:
//   these names are misleading, eg. the SHIFT_KEY bit
//   really means that an order gets queued instead of
//   executed immediately (a better name for it would
//   be QUEUED_ORDER), ALT_KEY in most contexts means
//   OVERRIDE_QUEUED_ORDER, etc.
//
#define META_KEY        (1 << 2) //   4
#define INTERNAL_ORDER  (1 << 3) //   8
#define RIGHT_MOUSE_KEY (1 << 4) //  16
#define SHIFT_KEY       (1 << 5) //  32
#define CONTROL_KEY     (1 << 6) //  64
#define ALT_KEY         (1 << 7) // 128

// maximum number of parameters for any (default and custom) command type
#define MAX_COMMAND_PARAMS 10


#ifdef __GNUC__
	#define _deprecated __attribute__ ((deprecated))
#else
	#define _deprecated
#endif

#if defined(BUILDING_AI)
#define CMD_DGUN CMD_MANUALFIRE
#endif

enum {
	MOVESTATE_NONE     = -1,
	MOVESTATE_HOLDPOS  =  0,
	MOVESTATE_MANEUVER =  1,
	MOVESTATE_ROAM     =  2,
};
enum {
	FIRESTATE_NONE          = -1,
	FIRESTATE_HOLDFIRE      =  0,
	FIRESTATE_RETURNFIRE    =  1,
	FIRESTATE_FIREATWILL    =  2,
	FIRESTATE_FIREATNEUTRAL =  3,
};




#if (defined(__cplusplus))
extern "C" {
#endif

	// this must have C linkage
	struct RawCommand {
		int id[2];
		int timeOut;

		unsigned int numParams;

		/// unique id within a CCommandQueue
		unsigned int tag;

		/// option bits (RIGHT_MOUSE_KEY, ...)
		unsigned char options;

		/// command parameters
		float* params;
	};

#if (defined(__cplusplus))
// extern "C"
};
#endif



struct Command
{
private:
	CR_DECLARE_STRUCT(Command)

public:
	Command()
	{
		memset(params, 0, sizeof(params));

		SetFlags(INT_MAX, 0, 0);
	}

	Command(const Command& c) {
		*this = c;
	}

	Command& operator = (const Command& c) {
		id[0]     = c.id[0];
		id[1]     = c.id[1];
		numParams = c.numParams;

		SetFlags(c.timeOut, c.tag, c.options);
		CopyParams(c);
		return *this;
	}

	Command(const float3& pos)
	{
		memset(params, 0, sizeof(params));

		PushPos(pos);
		SetFlags(INT_MAX, 0, 0);
	}

	Command(int cmdID)
	{
		memcpy(id, &cmdID, sizeof(cmdID));
		memset(params, 0, sizeof(params));

		SetFlags(INT_MAX, 0, 0);
	}

	Command(int cmdID, const float3& pos)
	{
		memcpy(id, &cmdID, sizeof(cmdID));
		memset(params, 0, sizeof(params));

		PushPos(pos);
		SetFlags(INT_MAX, 0, 0);
	}

	Command(int cmdID, unsigned char cmdOptions)
	{
		memcpy(id, &cmdID, sizeof(cmdID));
		memset(params, 0, sizeof(params));

		SetFlags(INT_MAX, 0, cmdOptions);
	}

	Command(int cmdID, unsigned char cmdOptions, float param)
	{
		memcpy(id, &cmdID, sizeof(cmdID));
		memset(params, 0, sizeof(params));

		PushParam(param);
		SetFlags(INT_MAX, 0, cmdOptions);
	}

	Command(int cmdID, unsigned char cmdOptions, const float3& pos)
	{
		memcpy(id, &cmdID, sizeof(cmdID));
		memset(params, 0, sizeof(params));

		PushPos(pos);
		SetFlags(INT_MAX, 0, cmdOptions);
	}

	Command(int cmdID, unsigned char cmdOptions, float param, const float3& pos)
	{
		memcpy(id, &cmdID, sizeof(cmdID));
		memset(params, 0, sizeof(params));

		PushParam(param);
		PushPos(pos);
		SetFlags(INT_MAX, 0, cmdOptions);
	}



	RawCommand ToRawCommand() {
		RawCommand rc;
		rc.id[0]   = id[0];
		rc.id[1]   = id[1];
		rc.timeOut = timeOut;

		rc.numParams   = numParams;
		rc.tag         = tag;
		rc.options     = options;
		rc.params      = &params[0];
		return rc;
	}

	void FromRawCommand(const RawCommand& rc) {
		id[0]     = rc.id[0];
		id[1]     = rc.id[1];
		numParams = rc.numParams;

		memset(params, 0, sizeof(params));
		SetFlags(rc.timeOut, rc.tag, rc.options);

		for (unsigned int n = 0; n < rc.numParams; n++) {
			PushParam(rc.params[n]);
		}
	}


	// returns true if the command references another object and
	// in this case also returns the param index of the object in cpos
	bool IsObjectCommand(int& cpos) const {
		const unsigned int psize = numParams;

		switch (GetID()) {
			case CMD_ATTACK:
			case CMD_FIGHT:
			case CMD_MANUALFIRE:
				cpos = 0;
				return (1 <= psize && psize < 3);
			case CMD_GUARD:
			case CMD_LOAD_ONTO:
				cpos = 0;
				return (psize >= 1);
			case CMD_CAPTURE:
			case CMD_LOAD_UNITS:
			case CMD_RECLAIM:
			case CMD_REPAIR:
			case CMD_RESURRECT:
				cpos = 0;
				return (1 <= psize && psize < 4);
			case CMD_UNLOAD_UNIT:
				cpos = 3;
				return (psize >= 4);
			case CMD_INSERT: {
				if (psize < 3)
					return false;

				Command icmd((int)params[1], (unsigned char)params[2]);

				for (unsigned int p = 3; p < psize; p++)
					icmd.PushParam(params[p]);

				if (!icmd.IsObjectCommand(cpos))
					return false;

				cpos += 3;
				return true;
			}
		}
		return false;
	}

	bool IsMoveCommand() const {
		switch (GetID()) {
			case CMD_AREA_ATTACK:
			case CMD_ATTACK:
			case CMD_CAPTURE:
			case CMD_FIGHT:
			case CMD_GUARD:
			case CMD_LOAD_UNITS:
			case CMD_MANUALFIRE:
			case CMD_MOVE:
			case CMD_PATROL:
			case CMD_RECLAIM:
			case CMD_REPAIR:
			case CMD_RESTORE:
			case CMD_RESURRECT:
			case CMD_UNLOAD_UNIT:
			case CMD_UNLOAD_UNITS:
				return true;

			case CMD_DEATHWAIT:
			case CMD_GATHERWAIT:
			case CMD_SELFD:
			case CMD_SQUADWAIT:
			case CMD_STOP:
			case CMD_TIMEWAIT:
			case CMD_WAIT:
				return false;

			default:
				// build commands are no different from reclaim or repair commands
				// in that they can require a unit to move, so return true when we
				// have one
				return IsBuildCommand();
		}
	}

	bool IsAreaCommand() const {
		switch (GetID()) {
			case CMD_CAPTURE:
			case CMD_LOAD_UNITS:
			case CMD_RECLAIM:
			case CMD_REPAIR:
			case CMD_RESURRECT:
				// params[0..2] always holds the position, params[3] the radius
				return (numParams == 4);
			case CMD_UNLOAD_UNITS:
				return (numParams == 5);
			case CMD_AREA_ATTACK:
				return true;
		}
		return false;
	}

	bool IsBuildCommand() const { return (GetID() < 0); }
	bool IsEmptyCommand() const { return (numParams == 0); }

	int GetID(bool idx = false) const { return id[idx]; }
	int GetTimeOut() const { return timeOut; }
	unsigned int GetNumParams() const { return numParams; }
	unsigned int GetTag() const { return tag; }
	unsigned char GetOpts() const { return options; }

	const float* GetParams() const { return &params[0]; }
	float GetParam(unsigned int idx) const { return ((idx >= numParams)? 0.0f: params[idx]); }


	bool SetParam(unsigned int idx, float param) {
		if (idx >= numParams)
			return false;
		params[idx] = param;
		return true;
	}
	bool PushParam(float param) {
		if (numParams >= MAX_COMMAND_PARAMS)
			return false;
		params[numParams++] = param;
		return true;
	}

	bool PushPos(const float3& pos) { return (PushPos(&pos.x)); }
	bool PushPos(const float* pos) {
		if ((numParams + 3) > MAX_COMMAND_PARAMS)
			return false;

		params[numParams++] = pos[0];
		params[numParams++] = pos[1];
		params[numParams++] = pos[2];
		return true;
	}

	float3 GetPos(unsigned int idx) const {
		float3 p;
		if ((idx + 3) > numParams)
			return p;
		p.x = params[idx + 0];
		p.y = params[idx + 1];
		p.z = params[idx + 2];
		return p;
	}

	bool SetPos(unsigned int idx, const float3& p) {
		if ((idx + 3) > numParams)
			return false;

		params[idx + 0] = p.x;
		params[idx + 1] = p.y;
		params[idx + 2] = p.z;
		return true;
	}

	void SetAICmdID(int cmdID) { id[1] = cmdID; }
	void SetTimeOut(int cmdTimeOut) { timeOut = cmdTimeOut; }
	void SetTag(unsigned int cmdTag) { tag = cmdTag; }
	void SetOpts(unsigned char cmdOpts) { options = cmdOpts; }
	void SetFlags(int cmdTimeOut, unsigned int cmdTag, unsigned char cmdOpts) {
		SetTimeOut(cmdTimeOut);
		SetTag(cmdTag);
		SetOpts(cmdOpts);
	}

	void CopyParams(const Command& c) {
		memset(params, 0, sizeof(params));
		memcpy(params, c.params, sizeof(params));
	}

private:
	/// [0] := CMD_xxx code (custom codes can also be used)
	/// [1] := AI Command callback id (passed in on handleCommand, returned in CommandFinished event)
	int id[2] = {0, -1};

	/**
	 * Remove this command after this frame (absolute).
	 * This can only be set locally and is not sent over the network.
	 * (used for temporary orders)
	 * Examples:
	 * - 0
	 * - MAX_INT
	 * - currenFrame + 60
	 */
	int timeOut = INT_MAX;

	unsigned int numParams = 0;

	/// unique id within a CCommandQueue
	unsigned int tag = 0;

	/// option bits (RIGHT_MOUSE_KEY, ...)
	unsigned char options = 0;

	/// command parameters
	float params[MAX_COMMAND_PARAMS];
};

#endif // COMMAND_H
