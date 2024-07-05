/**
* Interlocking library
* Author: Kyle Sarnik
**/

#pragma once

#include <CommonLib.h>

namespace ilock {

using lib::Map;
using lib::Vector;
using lib::String;
using lib::byte;

enum class LockState : byte
{
	On,
	Off
};

enum LockingRule : byte
{
	Unlocked,
	LockedAny,
	LockedOn,
	LockedOff
};

struct LockRuleTable
{
	LockingRule _lockedBy;
	LockingRule _locksWhenOn;
	LockingRule _locksWhenOff;
};

class Interlocking;

typedef byte LockingId;
typedef Map<LockingId, LockRuleTable> LockMap;

//! Base class for some locking mechanism, which interlocks with other mechanisms
class Locking
{
protected:
	LockingId _lid;
	LockState _state;
	Interlocking* _interlocking = nullptr;
	LockMap _lockingRules;
	bool _isLocked = false;
	Vector<LockingId> _curLockedBy;
	bool _lockingFinalized = false;
	String _name;

public:
	Locking() :
		_lid(0),
		_state(LockState::On),
		_name("unnamed")
	{}

	//! Base constructor, requires locking ID and interlocking ref
	Locking(LockingId lid, Interlocking& interlocking, const String& name) :
		_state(LockState::On),
		_lid(lid),
		_interlocking(&interlocking),
		_name(name)
	{}

	//! Get locking ID
	const LockingId& GetId() { return _lid; }

	//! Add interlocking rule to this locking mechanism
	void AddLockRule(const LockState state, const LockingId lid, const LockingRule rule);

	//! Apply a lock to this mechanism, locked by the specified ID
	void SetLock(const LockingId lockedBy, const LockingRule rule);

	//! Withdraw the lock on this mechanism applied by the specified ID
	void WithdrawLock(const LockingId lockedBy);

	//! Get whether this mechanism is currently locked
	bool IsLocked() { return _isLocked; }

	//! Get what is currently locking this mechanism
	const Vector<LockingId>& GetCurrentLockedBy() { return _curLockedBy; }

	//! Get parent interlocking
	const Interlocking* GetInterlocking() { return _interlocking; }

	//! Apply lock state, potentially ignoring locked state
	void ApplyLockState(LockState state, bool ignoreLocked = false);

	//! Get state of lock
	LockState GetState() { return _state; }

	//! Finalize all locks rules, should be invoked after adding all lock rules
	void FinalizeLockRules();

	//! Get name
	const String& GetName() { return _name; }

protected:
	//! Try to toggle the state of the mechanism. Returns whether it toggled. 
	bool TryToggleState();

private:
	//! Update status of the locked flag. Should be called any type a lock is set or withdrawn
	//! or whenever the lock state changes
	void UpdateLockStatus();

	//! Apply locks to all interlocked mechanisms
	void ApplyLocks(LockState state);

	//! Init a lock rule
	void InitLockRule(const LockingId& lid);
};

// Class for a interlocking lever
class Lever : public Locking
{
public:
	enum class State : byte
	{
		Normal,
		Reversed
	};

private:
	bool _isFaulted = false;
	State _leverState = State::Normal;

public:
	Lever(LockingId lid, Interlocking& interlocking, const String& name) 
		: Locking(lid, interlocking, name) {}

	//! Set lever state
	void SetLeverState(State newState);

	//! Throw lever, toggles the state
	void ThrowLever();

	//! Get whether the lever is faulted
	bool IsFaulted() { return _isFaulted; }

	//! Get current lever state
	const State GetLeverState() { return _leverState; }
};

typedef void (*LockChangedFunc)(LockingId, bool);
// Parent class of all locking mechanisms, manages faults and instantiation of mechanisms.
// LockingIds are not unqiue across interlocking instances.
class Interlocking
{
public:
	const static LockingId faultLockId = 0;

private:
	Map<String, LockingId> _lockNames;
	Map<LockingId, Locking*> _allLocks;
	Map<LockingId, bool> _faultedLevers;
	int _countFaulted = 0;
	Locking _faultLock;
	LockingId _nextId = 1;

	//! Callback function for when a lock state changes
	LockChangedFunc _onLockChange = nullptr;

public:
	Interlocking() :
		_faultLock(faultLockId, *this, "fault")
	{}

	//! Get lock mechanism by its id
	Locking* GetLocking(LockingId lid);

	//! Sets lever as faulted
	void SetLeverFaulted(LockingId id, bool faulted);

	//! Add lever with given name
	Lever* AddLever(String name);

	//! Add ancillary locking mechanism
	Locking* AddLocking(String name);

	//! Get locking mechanism by its name
	Locking* GetLocking(String name);

	//! Get all lockings
	Vector<LockingId> GetAllLockings();

	//! Set locking function callback
	void OnLockChange(LockChangedFunc func) { _onLockChange = func; }

	//! Invoke lock change callback
	void LockChange(LockingId id, bool locked);
};

} // namespace ilock
