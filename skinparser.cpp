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

// credit namazso @ unknowncheats.me

#include "skinparser.h"

#include <algorithm>
#include <codecvt>

#include "offsets.h"
#include "interfaces.h"

namespace SkinParser
{
	std::vector<std::pair<int, std::string>> k_skins;
	std::vector<std::pair<int, std::string>> k_gloves;
	std::vector<std::pair<int, std::string>> k_stickers;

	class CCStrike15ItemSchema;
	class CCStrike15ItemSystem;

	template <typename Key, typename Value>
	struct Node_t
	{
		int previous_id;		//0x0000
		int next_id;			//0x0004
		void* _unknown_ptr;		//0x0008
		int _unknown;			//0x000C
		Key key;				//0x0010
		Value value;			//0x0014
	};

	template <typename Key, typename Value>
	struct Head_t
	{
		Node_t<Key, Value>* memory;		//0x0000
		int allocation_count;			//0x0004
		int grow_size;					//0x0008
		int start_element;				//0x000C
		int next_available;				//0x0010
		int _unknown;					//0x0014
		int last_element;				//0x0018
	}; //Size=0x001C

	   // could use CUtlString but this is just easier and CUtlString isn't needed anywhere else
	struct String_t
	{
		char* buffer;	//0x0000
		int capacity;	//0x0004
		int grow_size;	//0x0008
		int length;		//0x000C
	}; //Size=0x0010

	struct CPaintKit
	{
		int id;						//0x0000

		String_t name;				//0x0004
		String_t description;		//0x0014
		String_t item_name;			//0x0024
		String_t material_name;		//0x0034
		String_t image_inventory;	//0x0044

		char pad_0x0054[0x8C];		//0x0054
	}; //Size=0x00E0

	struct CStickerKit
	{
		int id;

		int item_rarity;

		String_t name;
		String_t description;
		String_t item_name;
		String_t material_name;
		String_t image_inventory;

		int tournament_event_id;
		int tournament_team_id;
		int tournament_player_id;
		bool is_custom_sticker_material;

		float rotate_end;
		float rotate_start;

		float scale_min;
		float scale_max;

		float wear_min;
		float wear_max;

		String_t image_inventory2;
		String_t image_inventory_large;

		uint32_t pad0[4];
	};

	void InitializeKits()
	{
		static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

		// Search the relative calls

		// call    ItemSystem
		// push    dword ptr [esi+0Ch]
		// lea     ecx, [eax+4]
		// call    CEconItemSchema::GetPaintKitDefinition

		// Skip the opcode, read rel32 address
		auto item_system_offset = *reinterpret_cast<int32_t*>(Offsets::PaintkitSig + 1);

		// Add the offset to the end of the instruction
		auto item_system_fn = reinterpret_cast<CCStrike15ItemSystem* (*)()>(Offsets::PaintkitSig + 5 + item_system_offset);

		// Skip VTable, first member variable of ItemSystem is ItemSchema
		auto item_schema = reinterpret_cast<CCStrike15ItemSchema*>(uintptr_t(item_system_fn()) + sizeof(void*));

		// Dump paint kits
		{
			// Skip the instructions between, skip the opcode, read rel32 address
			auto get_paint_kit_definition_offset = *reinterpret_cast<int32_t*>(Offsets::PaintkitSig + 11 + 1);

			// Add the offset to the end of the instruction
			auto get_paint_kit_definition_fn = reinterpret_cast<CPaintKit* (__thiscall *)(CCStrike15ItemSchema*, int)>(Offsets::PaintkitSig + 11 + 5 + get_paint_kit_definition_offset);

			// The last offset is head_element, we need that

			// push    ebp
			// mov     ebp, esp
			// sub     esp, 0Ch
			// mov     eax, [ecx+298h]

			// Skip instructions, skip opcode, read offset
			auto start_element_offset = *reinterpret_cast<intptr_t*>(uintptr_t(get_paint_kit_definition_fn) + 8 + 2);

			// Calculate head base from start_element's offset
			auto head_offset = start_element_offset - 12;

			auto map_head = reinterpret_cast<Head_t<int, CPaintKit*>*>(uintptr_t(item_schema) + head_offset);

			for (int i = 0; i <= map_head->last_element; ++i)
			{
				auto paint_kit = map_head->memory[i].value;

				if (paint_kit->id == 9001)
					continue;

				const wchar_t* wide_name = I::Localize->Find(paint_kit->item_name.buffer + 1);
				auto name = converter.to_bytes(wide_name);

				if (paint_kit->id < 10000)
					k_skins.push_back({ paint_kit->id, name });
				else
					k_gloves.push_back({ paint_kit->id, name });
			}

			std::sort(k_skins.begin(), k_skins.end());
			std::sort(k_gloves.begin(), k_gloves.end());

			k_skins.at(0).second = "None";
		}

		// Dump sticker kits
		{
			// Skip the instructions between, skip the opcode, read rel32 address
			auto get_sticker_kit_definition_offset = *reinterpret_cast<intptr_t*>(Offsets::StickerSig + 1);

			// Add the offset to the end of the instruction
			auto get_sticker_kit_definition_fn = reinterpret_cast<CPaintKit* (__thiscall *)(CCStrike15ItemSchema*, int)>(Offsets::StickerSig + 5 + get_sticker_kit_definition_offset);

			// The last offset is head_element, we need that

			//	push    ebp
			//	mov     ebp, esp
			//	push    ebx
			//	push    esi
			//	push    edi
			//	mov     edi, ecx
			//	mov     eax, [edi + 2BCh]

			// Skip instructions, skip opcode, read offset
			auto start_element_offset = *reinterpret_cast<intptr_t*>(uintptr_t(get_sticker_kit_definition_fn) + 8 + 2);

			// Calculate head base from start_element's offset
			auto head_offset = start_element_offset - 12;

			auto map_head = reinterpret_cast<Head_t<int, CStickerKit*>*>(uintptr_t(item_schema) + head_offset);

			for (int i = 0; i <= map_head->last_element; ++i)
			{
				auto sticker_kit = map_head->memory[i].value;

				char sticker_name_if_valve_fucked_up_their_translations[64];

				auto sticker_name_ptr = sticker_kit->item_name.buffer + 1;

				if (strstr(sticker_name_ptr, "StickerKit_dhw2014_dignitas"))
				{
					strcpy_s(sticker_name_if_valve_fucked_up_their_translations, "StickerKit_dhw2014_teamdignitas");
					strcat_s(sticker_name_if_valve_fucked_up_their_translations, sticker_name_ptr + 27);
					sticker_name_ptr = sticker_name_if_valve_fucked_up_their_translations;
				}

				const wchar_t* wide_name = I::Localize->Find(sticker_name_ptr);
				auto name = converter.to_bytes(wide_name);

				k_stickers.push_back({ sticker_kit->id, name });
			}

			std::sort(k_stickers.begin(), k_stickers.end());

			k_stickers.at(0).second = "None";
		}
	}
}