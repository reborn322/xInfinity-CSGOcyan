/*MIT License
Copyright (c) namazso 2018 
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/

#include "stickers.h"
#include "skinchanger.h"

typedef float(__fastcall *AttributeFunction)(void*, void*, int, Stickers::EStickerAttributeType, float);
typedef unsigned int(__fastcall *IndexFunction)(void*, void*, int, Stickers::EStickerAttributeType, unsigned fl);

AttributeFunction oAttribFn;
IndexFunction oIndexFn;

float __fastcall AttributeFnHooked(void* thisptr, void*, int slot, Stickers::EStickerAttributeType attribute, float fl)
{
	C_BaseCombatWeapon* weapon = reinterpret_cast<C_BaseCombatWeapon*>((uintptr_t)thisptr - NetVars::CEconEntity::m_Item);

	switch (attribute)
	{
	case Stickers::EStickerAttributeType::Wear:
		return 0.000005f;
	case Stickers::EStickerAttributeType::Scale:
		return 1.f;
	case Stickers::EStickerAttributeType::Rotation:
		return 0.f;
	}

	return oAttribFn(thisptr, nullptr, slot, attribute, fl);
}

unsigned int __fastcall IndexFnHooked(void* thisptr, void*, int slot, Stickers::EStickerAttributeType attribute, unsigned fl)
{
	C_BaseCombatWeapon* weapon = reinterpret_cast<C_BaseCombatWeapon*>((uintptr_t)thisptr - NetVars::CEconEntity::m_Item);

	if (weapon && attribute == Stickers::EStickerAttributeType::Index)
	{
		int id = SkinChanger::GetSkinObjectByIndex(*weapon->GetItemDefinitionIndex())->GetStickerID(slot);

		if (id != 0)
			return id;
	}

	return oIndexFn(thisptr, nullptr, slot, attribute, fl);
}

void Stickers::ApplyStickerHooks(C_BaseAttributableItem* item)
{
	if (NetVars::CEconEntity::m_Item == 0xC)
		return;

	void**& stickerInfo = *reinterpret_cast<void***>((uintptr_t)item + NetVars::CEconEntity::m_Item);

	static void** stickerInfoHook = nullptr;

	if (!stickerInfoHook)
	{
		size_t length = 0;

		for (; stickerInfo[length]; ++length);

		stickerInfoHook = new void*[length];
		memcpy(stickerInfoHook, stickerInfo, length * sizeof(void*));

		oAttribFn = reinterpret_cast<AttributeFunction>(stickerInfoHook[4]);
		stickerInfoHook[4] = &AttributeFnHooked;

		oIndexFn = reinterpret_cast<IndexFunction>(stickerInfoHook[5]);
		stickerInfoHook[5] = &IndexFnHooked;
	}

	stickerInfo = stickerInfoHook;
}
