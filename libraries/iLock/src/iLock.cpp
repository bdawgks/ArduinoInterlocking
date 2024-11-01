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
	if (_lockingRules[lid]._lockedBy == Unlocked)
		return;

	InitLockRule(lid);
	_lockingRules[lid]._lockedBy = Unlocked;
	UpdateLockStatus();
}

void Locking::UpdateLockStatus()
{
	bool _prevIsLocked = _isLocked;
	_isLocked = false;
	_curLockedBy.clear();
	for (auto it = _lockingRules.begin(); it != _lockingRules.end(); it++)
	{
		LockingRule lockRule = it->second._lockedBy;
		if (lockRule == Unlocked)
			continue;

		// Check if lock rule applies to current state
		if ((_state == LockState::On && lockRule == LockedOn) ||
			(_state == LockState::Off && lockRule == LockedOff))
		{
			_isLocked = true;
			_curLockedBy.push_back(it->first);
		}
	}

	// Invoke callback
	if ((_isLocked && !_prevIsLocked) ||
		(!_isLocked && _prevIsLocked))
	{
		_interlocking->LockChange(_lid, _isLocked);
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

	_state = _state == LockState::On ? LockState::Off : LockState::On; // !_state;
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
			Serial.println("Lever " + other->GetName() + " lock withdrawn from " + GetName());
		}
		else
		{
			other->SetLock(_lid, rule);
			Serial.println("Lever " + other->GetName() + " lock applied from " + GetName());
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

bool Lever::SetLeverState(State newState)
{
	if (_leverState == newState)
		return false;

	if (!IsLocked() && newState != _state)
	{
		TryToggleState();
	}

	_leverState = newState;
	_isFaulted = _leverState != _state;
	_interlocking->SetLeverFaulted(_lid, _isFaulted);
	return !_isFaulted && !IsLocked();
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

Lever* Interlocking::GetLever(LockingId id)
{
	if (_allLevers.find(id) == _allLevers.end())
		return nullptr;

	return _allLevers[id];
}

void Interlocking::SetLeverFaulted(LockingId lever, bool faulted)
{
	/*if (faulted)
	{
		Serial.print("lever faulted: ");
		Serial.println(GetLocking(lever)->GetName());
	}*/

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
	_allLevers[_nextId] = lever;

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

void Interlocking::LockChange(LockingId id, bool locked)
{
	if (_onLockChange)
		_onLockChange(id, locked);
}

} // namespace ilock
	