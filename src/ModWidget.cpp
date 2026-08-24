#include "ModWidget.h"
#include <iostream>
#include <fstream>
#include <process.h>

static std::string file_name = "mods-settings.cwb";
static const int MODS_PER_PAGE = 7;
void* VTABLE[43];

mod::ModWidget* mod::ModWidget::ctor(cube::Game* game_ptr, plasma::Node* node_ptr, plasma::Node* background_ptr, std::vector<DLL*>* mods_ptr)
{
	// Construct basewidget
	std::wstring wstr_empty(L"");
	((cube::BaseWidget*)this)->ctor(game_ptr->plasma_engine, node_ptr, &wstr_empty);

	// Set fields
	this->game = game_ptr;
	this->hover_state = 0;
	this->background = background_ptr;
	this->mods = mods_ptr;
	this->page = 0;
	this->changed = false;

	// Set scalable font
	std::wstring fontName(L"resource1.dat");
	this->SetScalableFont(&fontName);
	this->Translate(100, 200, 1);

	// Copy real vtable populated by BaseWidget::ctor and only override Draw (slot 1)
	void** real_vtable = *(void***)this;
	for (int i = 0; i < 43; ++i)
	{
		this->artificial_vtable[i] = (real_vtable && real_vtable[i]) ? real_vtable[i] : VTABLE[i];
	}
	this->artificial_vtable[1] = (void*)Draw;

	// Manually set vtable
	size_t* vptr = (size_t*)this;
	*vptr = (size_t)this->artificial_vtable;

	return this;
}

void mod::ModWidget::MouseUp(cube::MouseButton mouse_button)
{
	if (mouse_button != cube::MouseButton::LeftMouseButton)
	{
		return;
	}

	switch (this->hover_state)
	{
	case HoverState::Exit:
		this->node->SetVisibility(false);
		if (this->changed)
		{
			// Restart the game
			char* argument_list[] = { (char *)"cubeworld.exe", nullptr };
			_execvp("cubeworld.exe", argument_list);
		}
		break;
	case HoverState::Toggle:
		if (this->selected < 0 || (size_t)this->selected >= this->mods->size())
		{
			return;
		}
		this->mods->at((size_t)this->selected)->enabled = !this->mods->at((size_t)this->selected)->enabled;
		ModWidget::StoreSave(this->mods);
		this->changed = true;
		break;
	case HoverState::Next:
		if (this->NextPageAvailable())
		{
			this->page++;
		}
		break;
	case HoverState::Previous:
		if (this->PreviousPageAvailable())
		{
			this->page--;
		}
		break;
	default:

		break;
	}
}

bool mod::ModWidget::NextPageAvailable()
{
	return ((size_t)this->page + 1) * MODS_PER_PAGE < this->mods->size();
}

bool mod::ModWidget::PreviousPageAvailable()
{
	return this->page > 0;
}

void mod::ModWidget::Draw(ModWidget* widget)
{
	const static float text_size = 18.0f; // Original	18.0f
	const static float border_size = 4.0f; // Original	4.0f
	const static float title_size = 25.0f;

	cube::Game* game = widget->game;

	FloatRGBA text_color(1.0f, 1.0f, 1.0f, 1.0f);
	FloatRGBA hover_color(0.2f, 1.0f, 1.0f, 1.0f);
	FloatRGBA warn_color(1.0f, 0.65f, 0.0f, 1.0f);
	FloatRGBA disabled_color(1.0f, 1.0f, 1.0f, 0.2f);
	FloatRGBA border_color(0.0f, 0.0f, 0.0f, 1.0f);

	FloatVector2 mouse_pos;
	FloatVector2 pos(0, 0);
	FloatVector2 size(500, 500);

	std::wstring wstr_title(L"Mods");
	std::wstring wstr_reminder(L"If you change something \n the game restarts to \n reload all the mods!");
	std::wstring wstr_next(L">");
	std::wstring wstr_prev(L"<");
	std::wstring wstr_x(L"X");

	// Translate background and node
	widget->node->Translate((float)game->width / 2.0f, (float)game->height / 2.0f, -size.x / 2.0f, -size.y / 2.0f);
	widget->background->Translate((float)game->width / 2.0f, (float)game->height / 2.0f, -size.x / 2.0f, -size.y / 2.0f);

	// Scale background and node
	widget->SetSize(&size);
	widget->background->widget1->SetSize(&size);

	// Get mouse position in the widget
	mouse_pos = *widget->GetRelativeMousePosition(&mouse_pos);

	// Set hover to 0
	widget->hover_state = ModWidget::HoverState::None;

	// Text settings
	widget->SetTextSize(text_size);
	widget->SetTextColor(&text_color);
	widget->SetBorderSize(border_size);
	widget->SetBorderColor(&border_color);
	widget->SetTextPivot(plasma::TextPivot::Center);

	// Draw title
	widget->SetTextSize(title_size);
	widget->DrawString(&pos, &wstr_title, size.x/2, 20 + text_size);

	// Draw x to exit
	widget->SetTextPivot(plasma::TextPivot::Right);
	if (plasma::Widget::IsSquareHovered(&mouse_pos, (int)size.x - 30, 20, 20, 30))
	{
		widget->SetTextColor(&hover_color);
		widget->hover_state = ModWidget::HoverState::Exit;
	}
	widget->DrawString(&pos, &wstr_x, size.x - 10, 20 + text_size);

	// Draw reminder
	widget->SetTextColor(&warn_color);
	widget->SetTextSize(text_size - 5);
	widget->SetTextPivot(plasma::TextPivot::Center);
	widget->DrawString(&pos, &wstr_reminder, size.x / 2, 2 * (10 + text_size));

	// Draw mods
	int y_count = 0;

	for (int i = widget->page * 7; i < (widget->page + 1)*7 && (size_t)i < widget->mods->size(); i++)
	{
		DLL* dll = widget->mods->at(i);
		widget->SetTextPivot(plasma::TextPivot::Left);
		widget->SetTextColor(&text_color);
		if (!dll->enabled)
		{
			widget->SetTextColor(&disabled_color);
		}

		float y_pos = (4.0f + 2.0f * y_count) * (10.0f + text_size);
		if (plasma::Widget::IsSquareHovered(&mouse_pos, 0, (int)y_pos - 20, (int)size.x, 30))
		{
			widget->SetTextColor(&hover_color);
			widget->hover_state = HoverState::Toggle;
			widget->selected = i;
		}
		std::wstring name = L"- " + std::wstring(dll->fileName.begin() + 5, dll->fileName.end());
		if (name.size() > 45)
		{
			name = name.substr(0, 42) + L"...";
		}
		widget->DrawString(&pos, &name, 20.0f, y_pos);

		y_count++;
		if (y_pos > size.y - 2 * (10 + text_size))
		{
			break;
		}
	}

	// Draw prev button
	widget->SetTextColor(&text_color);
	if (!widget->PreviousPageAvailable())
	{
		widget->SetTextColor(&disabled_color);
	}
	else if (plasma::Widget::IsSquareHovered(&mouse_pos, 20, (int)(size.y - text_size - 20.0f), 20, 30))
	{
		widget->hover_state = HoverState::Previous;
		widget->SetTextColor(&hover_color);
	}
	widget->DrawString(&pos, &wstr_prev, 20.0f, size.y - text_size);

	// Draw next button
	widget->SetTextPivot(plasma::TextPivot::Right);
	widget->SetTextColor(&text_color);

	if (!widget->NextPageAvailable())
	{
		widget->SetTextColor(&disabled_color);
	}
	else if (plasma::Widget::IsSquareHovered(&mouse_pos, (int)(size.x - text_size - 20.0f), (int)(size.y - text_size - 20.0f), 20, 30))
	{
		widget->hover_state = HoverState::Next;
		widget->SetTextColor(&hover_color);
	}
	widget->DrawString(&pos, &wstr_next, size.x - 20, size.y - text_size);

	// Draw current page
	widget->SetTextColor(&text_color);
	widget->SetTextPivot(plasma::TextPivot::Center);
	std::wstring page = std::to_wstring(widget->page + 1) + L"/" + std::to_wstring((int)(widget->mods->size() / MODS_PER_PAGE) + 1);
	widget->DrawString(&pos, &page, size.x / 2, size.y - text_size);
}

void mod::ModWidget::Init()
{
	void* NULLSUB = CWOffset(0xE8A20);
	void* RETZERO = CWOffset(0x368230);

	void* vtab[43] = {
	CWOffset(0x268B40),
	(void*)Draw,				// void Draw(plasma::Widget*)
	CWOffset(0x26A720),
	CWOffset(0x26A720),
	RETZERO,
	RETZERO,
	NULLSUB,
	CWOffset(0x32B830),
	CWOffset(0x32BFD0),
	CWOffset(0x32BD70),
	NULLSUB,
	CWOffset(0x32B990),
	CWOffset(0x32BB40),
	NULLSUB,
	NULLSUB,
	NULLSUB,
	NULLSUB,
	NULLSUB,
	NULLSUB,
	NULLSUB,
	NULLSUB,
	CWOffset(0x32BB80),		// void OnMouseOver(plasma::Widget*)
	NULLSUB,
	NULLSUB,
	NULLSUB,
	NULLSUB,
	NULLSUB,
	NULLSUB,
	NULLSUB,
	NULLSUB,
	NULLSUB,
	NULLSUB,
	NULLSUB,
	NULLSUB,
	NULLSUB,
	CWOffset(0x32B5A0),
	CWOffset(0x32B6B0),
	NULLSUB,
	NULLSUB,
	CWOffset(0x32B6E0),
	CWOffset(0x32A2C0),		// plasma::Widget* CreateCopy(plasma::Widget*);
	CWOffset(0x32A8D0),
	NULLSUB
	};

	for (int i = 0; i < 43; ++i)
	{
		VTABLE[i] = vtab[i];
	}
}

void mod::ModWidget::LoadSave(std::vector<DLL*>* mods)
{
	if (!mods)
	{
		return;
	}

	std::ifstream in(file_name.c_str());
	if (!in.is_open())
	{
		return;
	}

	std::string line;
	while (std::getline(in, line)) {
		if (line.empty())
		{
			continue;
		}

		auto pos = line.find(':');
		if (pos != std::string::npos)
		{
			std::string name = line.substr(0, pos);
			std::string valStr = line.substr(pos + 1);

			bool enabled = false;
			try {
				enabled = (std::stoi(valStr) != 0);
			} catch (...) {
				// Ignore corrupted line and continue parsing
				continue;
			}

			for (DLL* dll : *mods)
			{
				if (dll && name == dll->fileName)
				{
					dll->enabled = enabled;
				}
			}
		}
	}
}

void mod::ModWidget::StoreSave(std::vector<DLL*>* mods)
{
	if (!mods)
	{
		return;
	}

	std::ofstream out(file_name.c_str());
	if (!out.is_open())
	{
		return;
	}

	for (DLL* dll : *mods)
	{
		if (dll)
		{
			out << dll->fileName << ":" << (dll->enabled ? 1 : 0) << "\n";
		}
	}
}
