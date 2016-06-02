/*
 * Copyright (c) 2015, 2016 arduino-menusystem
 * Licensed under the MIT license (see LICENSE)
 */

#include "MenuSystem.h"
#include <stdlib.h>


// *********************************************************
// MenuComponent
// *********************************************************

MenuComponent::MenuComponent(const char* name)
: _name(name)
{
}

const char* MenuComponent::get_name() const
{
    return _name;
}

String& MenuComponent::get_composite_name(String& buffer) const
{
    // this replaces the content of buffer
    buffer = _name;
    return buffer;
}

void MenuComponent::set_name(const char* name)
{
    _name = name;
}


// *********************************************************
// Menu
// *********************************************************

Menu::Menu(const char* name, void (*on_display)(Menu*))
: MenuComponent(name),
  _on_display(on_display),
  _p_sel_menu_component(NULL),
  _menu_components(NULL),
  _p_parent(NULL),
  _num_menu_components(0),
  _cur_menu_component_num(0),
  _prev_menu_component_num(0)
{
}

bool Menu::display()
{
    if (_on_display)
    {
        _on_display(this);
        return true;
    }
    return false;
}

void Menu::set_display_function(void (*on_display)(Menu*))
{
    _on_display = on_display;
}

bool Menu::next(bool loop)
{
    _prev_menu_component_num = _cur_menu_component_num;

    if (!_num_menu_components)
    {
        return false;
    }
    else if (_cur_menu_component_num != _num_menu_components - 1)
    {
        _cur_menu_component_num++;
        _p_sel_menu_component = _menu_components[_cur_menu_component_num];

        return true;
    }
    else if (loop)
    {
        _cur_menu_component_num = 0;
        _p_sel_menu_component = _menu_components[_cur_menu_component_num];

        return true;
    }
    return false;
}

bool Menu::prev(bool loop)
{
    _prev_menu_component_num = _cur_menu_component_num;

    if (!_num_menu_components)
    {
        return false;
    }
    else if (_cur_menu_component_num != 0)
    {
        _cur_menu_component_num--;
        _p_sel_menu_component = _menu_components[_cur_menu_component_num];

        return true;
    }
    else if (loop)
    {
        _cur_menu_component_num = _num_menu_components - 1;
        _p_sel_menu_component = _menu_components[_cur_menu_component_num];

        return true;
    }
    return false;
}

MenuComponent* Menu::activate()
{
    if (!_num_menu_components)
        return NULL;

    MenuComponent* pComponent = _menu_components[_cur_menu_component_num];

    if (pComponent == NULL)
        return NULL;

    return pComponent->select();
}

MenuComponent* Menu::select()
{
    return this;
}

void Menu::reset()
{
    for (int i = 0; i < _num_menu_components; ++i)
        _menu_components[i]->reset();

    _prev_menu_component_num = 0;
    _cur_menu_component_num = 0;
    _p_sel_menu_component = _num_menu_components ? _menu_components[0] : NULL;
}

void Menu::add_item(MenuItem* pItem, void (*on_select)(MenuItem*))
{
    // Resize menu component list, keeping existing items.
    // If it fails, there the item is not added and the function returns.
    _menu_components = (MenuComponent**) realloc(_menu_components,
                                                 (_num_menu_components + 1)
                                                 * sizeof(MenuComponent*));
    if (_menu_components == NULL)
      return;

    _menu_components[_num_menu_components] = pItem;

    pItem->set_select_function(on_select);

    if (_num_menu_components == 0)
        _p_sel_menu_component = pItem;

    _num_menu_components++;
}

Menu const* Menu::get_parent() const
{
    return _p_parent;
}

void Menu::set_parent(Menu* pParent)
{
    _p_parent = pParent;
}

Menu const* Menu::add_menu(Menu* pMenu)
{
    // Resize menu component list, keeping existing items.
    // If it fails, there the item is not added and the function returns.
    _menu_components = (MenuComponent**) realloc(_menu_components,
                                                 (_num_menu_components + 1)
                                                 * sizeof(MenuComponent*));
    if (_menu_components == NULL)
      return NULL;

    pMenu->set_parent(this);

    _menu_components[_num_menu_components] = pMenu;

    if (_num_menu_components == 0)
        _p_sel_menu_component = pMenu;

    _num_menu_components++;

    return pMenu;
}

MenuComponent const* Menu::get_menu_component(byte index) const
{
  return _menu_components[index];
}

MenuComponent const* Menu::get_selected() const
{
    return _p_sel_menu_component;
}

byte Menu::get_num_menu_components() const
{
    return _num_menu_components;
}

byte Menu::get_cur_menu_component_num() const
{
    return _cur_menu_component_num;
}

byte Menu::get_prev_menu_component_num() const
{
    return _prev_menu_component_num;
}

// *********************************************************
// BackMenuItem
// *********************************************************
BackMenuItem::BackMenuItem(MenuSystem* ms, const char* name)
: MenuItem(name),
  menu_system(ms)
{
}

MenuComponent* BackMenuItem::select()
{
    if (_on_select!=NULL)
        _on_select(this);

    if (menu_system!=NULL)
        menu_system->back();

    return NULL;
}

// *********************************************************
// MenuItem
// *********************************************************

MenuItem::MenuItem(const char* name)
: MenuComponent(name),
  _on_select(0)
{
}

void MenuItem::set_select_function(void (*on_select)(MenuItem*))
{
    _on_select = on_select;
}

MenuComponent* MenuItem::select()
{
    if (_on_select != NULL)
        _on_select(this);

    return NULL;
}

void MenuItem::reset()
{
    // Do nothing.
}

// *********************************************************
// NumericMenuItem
// *********************************************************

NumericMenuItem::NumericMenuItem(const char* basename, float value,
                                 float minValue, float maxValue,
                                 float increment,
                                 ValueFormatter_t value_formatter)
: MenuItem(basename),
  _value(value),
  _minValue(minValue),
  _maxValue(maxValue),
  _increment(increment),
  _editing_value(false),
  _value_formatter(value_formatter)
{
    if (_increment < 0.0) _increment = -_increment;
    if (_minValue > _maxValue)
    {
        float tmp = _maxValue;
        _maxValue = _minValue;
        _minValue = tmp;
    }
};

void NumericMenuItem::set_number_formatter(ValueFormatter_t value_formatter)
{
    _value_formatter = value_formatter;
}

bool NumericMenuItem::is_editing_value() const
{
    return _editing_value;
}

MenuComponent* NumericMenuItem::select()
{
    _editing_value = !_editing_value;

    // Only run _on_select when the user is done editing the value
    if (!_editing_value && _on_select != NULL)
        _on_select(this);
    return NULL;
}

bool NumericMenuItem::next_value()
{
    _value += _increment;
    if (_value > _maxValue)
        _value = _maxValue;
    return true;
}

bool NumericMenuItem::prev_value()
{
    _value -= _increment;
    if (_value < _minValue)
        _value = _minValue;
    return true;
}

String& NumericMenuItem::get_composite_name(String& buffer) const
{
    buffer = _name;
    buffer += is_editing_value() ? '<' : '=';

    if (_value_formatter != NULL)
        buffer += _value_formatter(_value);
    else
        buffer += _value;

    if (is_editing_value())
        buffer += '>';

    return buffer;
}

// *********************************************************
// MenuSystem
// *********************************************************

MenuSystem::MenuSystem()
: _p_root_menu(NULL),
  _p_curr_menu(NULL)
{
}

bool MenuSystem::next(bool loop)
{
    if (_p_curr_menu->get_selected()->is_editing_value())
    {
        _p_curr_menu->_p_sel_menu_component->next_value();
        return true;
    }
    else
    {
        return _p_curr_menu->next(loop);
    }
}

bool MenuSystem::prev(bool loop)
{
    if (_p_curr_menu->get_selected()->is_editing_value())
    {
        _p_curr_menu->_p_sel_menu_component->prev_value();
        return true;
    }
    else
    {
        return _p_curr_menu->prev(loop);
    }
}

void MenuSystem::reset()
{
    _p_curr_menu = _p_root_menu;
    _p_root_menu->reset();
}

void MenuSystem::select(bool reset)
{
    MenuComponent* pComponent = _p_curr_menu->activate();

    if (pComponent != NULL)
        _p_curr_menu = (Menu*) pComponent;
    else
        if (reset)
            this->reset();
}

bool MenuSystem::back()
{
    if (_p_curr_menu != _p_root_menu)
    {
        _p_curr_menu = const_cast<Menu*>(_p_curr_menu->get_parent());
        return true;
    }

    // We are already in the root menu
    return false;
}

void MenuSystem::set_root_menu(Menu* p_root_menu)
{
    _p_root_menu = p_root_menu;
    _p_curr_menu = p_root_menu;
}

Menu const* MenuSystem::get_current_menu() const
{
    return _p_curr_menu;
}

bool MenuSystem::display()
{
    if (_p_curr_menu != NULL)
        return _p_curr_menu->display();
    return false;
}
