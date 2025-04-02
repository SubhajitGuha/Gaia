#pragma once
#include "Gaia/Core.h"
#include "Gaia/Log.h"
#include "Gaia//Renderer/GaiaRenderer.h"
#include <vector>

#define END_OF_LIST 0xffffffffu

namespace Gaia
{
	template<typename ObjectType>
	class Handle;

	template<typename ObjectType, typename ImplObjectType>
	class Pool {

		struct PoolEntry
		{
			explicit PoolEntry(ImplObjectType& obj) : obj_(std::move(obj)) {}
			ImplObjectType obj_ = {}; //default
			uint32_t gen_ = 1; //generation is incremented whenever we re-use the same index
			uint32_t nextFreeIndex_ = END_OF_LIST; //next free location in the array
		};

	public:
		std::vector<PoolEntry> objects_;
		uint32_t numObjects_ = 0u;
		uint32_t freeListHead_ = END_OF_LIST; //points to the index of object array which is freed

		Handle<ObjectType> Create(ImplObjectType&& obj)
		{
			uint32_t idx = 0;
			if (freeListHead_ != END_OF_LIST)
			{
				idx = freeListHead_;
				freeListHead_ = objects_[idx].nextFreeIndex_; //list head now becomes the next free index
				objects_[idx].obj_ = std::move(obj);
			}
			else
			{
				idx = objects_.size();
				objects_.emplace_back(obj);
			}
			numObjects_++;
			return Handle<ObjectType>(idx, objects_[idx].gen_);
		}

		void Destroy(Handle<ObjectType> handle)
		{
			if (handle.isEmpty())
				return;
			GAIA_ASSERT(numObjects_ > 0, "");
			uint32_t index = handle.index();
			GAIA_ASSERT(index < objects_.size(), "");
			GAIA_ASSERT(handle.gen() == objects_[index].gen_, "");

			objects_[index].obj_ = ImplObjectType{};
			objects_[index].gen_++;
			objects_[index].nextFreeIndex_ = freeListHead_;
			freeListHead_ = index;
			numObjects_--;
		}

		ImplObjectType* get(Handle<ObjectType> handle)
		{
			if (handle.isEmpty())
				return nullptr;
			uint32_t index = handle.index();
			GAIA_ASSERT(index < objects_.size(), "");
			GAIA_ASSERT(handle.gen() == objects_[index].gen_, "");
			return &objects_[index].obj_;
		}

		Handle<ObjectType> findObject(const ImplObjectType* obj)
		{
			if (!obj)
			{
				return {};
			}

			for (int i = 0; i < objects_.size(); i++)
			{
				if (objects_[i].obj_ == *obj)
				{
					return Handle<ObjectType>((uint32_t)i, objects_[i].gen_);
				}
			}
		}

		bool isPresent(ImplObjectType* obj)
		{
			for (auto& poolObj : objects_)
			{
				int val = memcmp(&poolObj.obj_, &obj, sizeof(poolObj.obj_));
				if (val == 0)
					return true;
			}
			return false;
		}

		void Clear()
		{
			objects_.clear();
			numObjects_ = 0;
			freeListHead_ = END_OF_LIST;
		}
	};
}