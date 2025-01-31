#pragma once
#include"Event.h"

#include<sstream>

namespace Gaia {
	class KeyEvent : public Event {
	public:
		inline int GetKeyCode() { return m_KeyCode; }

		EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
	protected:
		KeyEvent(int KeyCode) 
			:m_KeyCode(KeyCode){}
		int m_KeyCode;
	};

	class KeyPressedEvent : public KeyEvent
	{
	public:
		KeyPressedEvent(int KeyCode, int repeatCount)
			:KeyEvent(KeyCode),m_RepeatCount(repeatCount) {}

		inline int GetRepeatCount()const { return m_RepeatCount; }

		std::string ToString()const override {//for logging
			std::stringstream ss;
			ss << "Key Pressed Event" << m_KeyCode << " , " << m_RepeatCount;
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyPressed)

	private:
		int m_RepeatCount;
	};

	class KeyReleasedEvent :public KeyEvent
	{
	public:
		KeyReleasedEvent(int KeyCode)
			:KeyEvent(KeyCode) {}
		std::string ToString()const override {
			std::stringstream ss;
			ss << "Key Released Event" << m_KeyCode;
			return ss.str();
 		}
		EVENT_CLASS_TYPE(KeyReleased)
	};

	class KeyTypedEvent : public KeyEvent
	{
	public:
		KeyTypedEvent(int KeyCode)
			:KeyEvent(KeyCode){}

		std::string ToString()const override {//for logging
			std::stringstream ss;
			ss << "Key Typed Event" << m_KeyCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyTyped)

	};
}