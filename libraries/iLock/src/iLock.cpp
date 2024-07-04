/**
* Interlocking library
* Author: Kyle Sarnik
**/

#include "iLock.h"

namespace ilock
{

#pragma region Operators
const LockState& operator!(LockState& orig)
{
	if (orig == LockState::On)
		return LockState::Off;
	else
		return LockState::On;
}

const Lever::State& operator!(Lever::State& orig)
{
	if (orig == Lever::State::Normal)
		return Lever::State::Reversed;
	else
		return Lever::State::Normal;
}

bool operator==(const Lever::State& lhs, const LockState& rhs)
{
	if (lhs == Lever::State::Normal && rhs == LockState::On)
		return true;
	else if (lhs == Lever::State::Reversed && rhs == LockState::Off)
		return true;

	return false;
}

bool operator!=(const Lever::State& lhs, const LockState& rhs)
{
	return !(lhs == rhs);
}

bool operator==(const LockState& lhs, const Lever::State& rhs)
{
	return rhs == lhs;
}

bool operator!=(const LockState& lhs, const Lever::State& rhs)
{
	return rhs != lhs;
}
#pragma endregion Operators

void Locking::InitLockRule(const LockingId& lid)
{
	if (_lockingRules.find(lid) == _lockingRules.end())
	{
		LockRuleTable rules = LockRuleTable{ Unlocked, Unlocked, Unlocked };
		_lockingRules.insert(std::make_pair(lid, rules));
	}
}

void Locking::AddLockRule(const LockState state, const LockingId lid, const LockingRule rule)
{
	if (_lockingFinalized)
		return;

	InitLockRule(lid);

	if (state == LockState::On)
	{
		_lockingRules[lid]._locksWhenOn = rule;
	}
	else if (state == LockState::Off)
	{
		_lockingRules[lid]._locksWhenOff = rule;
	}
}

void Locking::SetLock(const LockingId lid, const LockingRule rule)
{
	if (rule == Unlocked)
		return;

	InitLockRule(lid);
	_lockingRules[lid]._lockedBy = rule;
	UpdateLockStatus();
}

void Locking::WithdrawLock(const LockingId lid)
{
	if (_lockingRules[lid]._lockedBy = Unlocked)
		return;

	InitLockRule(lid);
	_lockingRules[lid]._lockedBy = Unlocked;
	UpdateLockStatus();
}

void Locking::UpdateLockStatus()
{
	_isLocked = false;
	_curLockedBy.clear();
	for (auto it = _lockingRules.begin(); it != _lockingRules.end(); it++)
	{
		if (it->second._lockedBy != Unlocked)
		{
			_isLocked = true;
			_curLockedBy.push_back(it->first);
		}
	}
}

void Locking::ApplyLockState(LockState state, bool ignoreLocked)
{
	if (_isLocked && !ignoreLocked)
		return;

	_state = state;
	ApplyLocks(_state);
}

bool Locking::TryToggleState()
{
	if (_isLocked)
		return false;

	_state = !_state;
	ApplyLocks(_state);

	return true;
}


void Locking::ApplyLocks(LockState state)
{
	for (auto it = _lockingRules.begin(); it != _lockingRules.end(); it++)
	{
		Locking* other = _interlocking->GetLocking(it->first);
		if (!other)
			continue;

		LockingRule rule = Unlocked;
		if (state == LockState::On)
			rule = it->second._locksWhenOn;
		else if (state == LockState::Off)
			rule = it->second._locksWhenOff;

		if (rule == Unlocked)
		{
			other->WithdrawLock(_lid);
		}
		else
		{
			other->SetLock(_lid, rule);
		}
	}
}

void Locking::FinalizeLockRules()
{
	if (_lockingFinalized)
		return;

	ApplyLocks(_state);

	_lockingFinalized = true;
}

void Lever::SetLeverState(State newState)
{
	if (_leverState == newState)
		return;

	if (!IsLocked() && newState != _state)
	{
		TryToggleState();
	}

	_leverState = newState;
	_isFaulted = _leverState != _state;
	_interlocking->SetLeverFaulted(_lid, _isFaulted);
}

void Lever::ThrowLever()
{
	TryToggleState();
	_leverState = !_leverState;
	_isFaulted = _leverState != _state;
	_interlocking->SetLeverFaulted(_lid, _isFaulted);
}

Locking* Interlocking::GetLocking(LockingId id)
{
	if (id == faultLockId)
		return &_faultLock;

	if (_allLocks.find(id) == _allLocks.end())
		return nullptr;

	return _allLocks[id];
}

void Interlocking::SetLeverFaulted(LockingId lever, bool faulted)
{
	bool currentlyFaulted = false;
	if (_faultedLevers.find(lever) != _faultedLevers.end())
	{
		currentlyFaulted = _faultedLevers[lever];
	}

	// Update faulted count only if status changed
	if (faulted && !currentlyFaulted)
	{
		_countFaulted++;
	}
	else if (!faulted && currentlyFaulted)
	{
		_countFaulted--;
	}

	// Keep track of fault status
	_faultedLevers[lever] = faulted;

	// Apply lock
	if (_countFaulted > 0)
	{
		_faultLock.ApplyLockState(LockState::On, true);
	}
	else
	{
		_faultLock.ApplyLockState(LockState::Off, true);
	}
}

Lever* Interlocking::AddLever(String name)
{
	_lockNames.insert(std::make_pair(name, _nextId));
	Lever* lever = new Lever(_nextId, *this, name);
	_allLocks[_nextId] = lever;

	// Make this lever locked when the fault lock is on
	_faultLock.AddLockRule(LockState::On, _nextId, LockedAny);
	_faultLock.AddLockRule(LockState::Off, _nextId, Unlocked);

	// Add to fault map
	_faultedLevers.insert(std::make_pair(_nextId, false));

	// Increment the id before returning
	_nextId++;
	return lever;
}

Locking* Interlocking::AddLocking(String name)
{
	_lockNames.insert(std::make_pair(name, _nextId));
	Locking* locking = new Locking(_nextId, *this, name);

	// Increment the id after returning it
	_allLocks[_nextId++] = locking;
	return locking;
}

Locking* Interlocking::GetLocking(String name)
{
	if (_lockNames.find(name) == _lockNames.end())
		return nullptr;

	return GetLocking(_lockNames[name]);
}

Vector<LockingId> Interlocking::GetAllLockings()
{
	Vector<LockingId> ids = Vector<LockingId>();
	for (auto it = _allLocks.begin(); it != _allLocks.end(); it++)
	{
		ids.push_back(it->first);
	}
	return ids;
}

} // namespace ilock
	