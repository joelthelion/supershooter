#include "gui.h"

#include <cmath>
#include <cassert>
#include "except.h"
#include "sound.h"

bool Widget::is_click_valid(const Sprite *sprite,float x, float y) {
    float bx = -sprite->w/2., by = -sprite->h/2.;
    float tx = sprite->w/2.,  ty = sprite->h/2.;
    float a,fx,fy;

    sprite->absolute_coordinates(bx,by,a,fx,fy);
    assert(a==0.);
    sprite->absolute_coordinates(tx,ty,a,fx,fy);
    assert(a==0.);

    assert(bx<tx and by<ty);

    if (x<bx or x>tx or y<by or y>ty) return false;
    return true;
}

Widget::Widget() : data(NULL), enabled(true) {};
Widget::~Widget() {};

Group::Group() : Widget() {};
Group::~Group() {
    for (Widgets::iterator i=widgets.begin(); i!=widgets.end(); i++) { delete i->second; }
    widgets.clear(); //unusefull?
}

void Group::add_widget(Widget *widget,const std::string &name) {
    Widgets::iterator i = widgets.find(name);
    if (i!=widgets.end()) { delete i->second; widgets.erase(i); }
    widgets[name] = widget;
}

Widget *Group::get_widget(const std::string &name) {
    Widgets::iterator i = widgets.find(name);
    if (i!=widgets.end()) return i->second;
    return NULL;
}

bool Group::interact(float x, float y) {
    if (not enabled) return false;

    bool ret = false;
    for (Widgets::iterator i=widgets.begin(); i!=widgets.end(); i++) {
        Widget *widget = i->second;
        ret |= widget->interact(x,y);
    }
    return ret;
}

bool Group::draw(float x,float y,float dt) const {
    if (not enabled) return false;
    bool any = false;
    for (Widgets::const_iterator i=widgets.begin(); i!=widgets.end(); i++) {
        Widget *widget = i->second;
        any |= widget->draw(x,y,dt);
    }
    return any;
}

Array::Array(int nw, int nh) : nw(nw), nh(nh), size(nw*nh) {
    widgets = new Widget*[size];
    for (int k=0; k<size; k++) { widgets[k] = NULL; }
}

Array::~Array() {
    for (int k=0; k<size; k++) if (widgets[k]!=NULL) { delete widgets[k]; widgets[k] = NULL; }
    delete [] widgets;
}

void Array::add_widget(Widget *widget,int row,int column) {
    if (row<0 or row>=nh or column<0 or column>=nw) {
        delete widget;
        return;
    }

    Widget *&current = unflatten(row,column);
    if (current) {
        delete current;
        current = NULL;
    }

    assert(not current);
    current = widget;
}

Widget *Array::get_widget(int row,int column) {
    if (row<0 or row>=nh or column<0 or column>=nw) return NULL;
    return unflatten(row,column);
}

bool Array::interact(float x, float y) {
    if (not enabled) return false;

    bool ret = false;
    for (int k=0; k<size; k++) {
        Widget *widget = widgets[k];
        if (widget) ret |= widget->interact(x,y);
    }
    return ret;
}

bool Array::draw(float x,float y,float dt) const {
    if (not enabled) return false;
    bool any = false;
    for (int k=0; k<size; k++) {
        const Widget *widget = widgets[k];
        if (widget) any |= widget->draw(x,y,dt);
    }
    return any;
}

Widget *&Array::unflatten(int row,int column) { return widgets[row+column*nh]; }

Button::Button(Sprite *sprite, void (*clicked)(Button*)) : Widget(), sprite(sprite), clicked(clicked) { assert(sprite); }
Button::Button(const std::string &sprname, void (*clicked)(Button*)) : Widget(), sprite(SpriteManager::get()->get_sprite(sprname)), clicked(clicked) { assert(sprite); }
Button::~Button() { delete sprite; }

bool Button::interact(float x, float y) {
    if (not enabled) return false;

    bool valid = is_click_valid(sprite,x,y);
    if (valid and clicked) clicked(this);
    return valid;
}

bool Button::draw(float x,float y,float dt) const {
    if (not enabled) return false;
    sprite->draw(dt);
    return true;
}

ToggleButton::ToggleButton(const std::string &sprname, void (*toggled)(Button*), bool istate, const std::string &text, const std::string &font, Text::Align align) : Button(sprname,toggled), state(istate), label(NULL) {
    if (text!=std::string()) { label = SpriteManager::get()->get_text(text,font,align); }
    casted = dynamic_cast<StateSprite*>(sprite);
    assert(casted and casted->nstate>=2);
    casted->state = state;
}
ToggleButton::~ToggleButton() {
    if (label) delete label;
}

bool ToggleButton::interact(float x, float y) {
    if (not Button::interact(x,y)) return false;

    state = !state;
    casted->state = state;
    return true;
}

bool ToggleButton::draw(float x,float y,float dt) const {
    if (not Button::draw(x,y,dt)) return false;
    if (label) {
        label->draw(dt);
        label->y = sprite->y;
        label->x = sprite->x + sprite->w;
        label->z = sprite->z;
        label->update_z();
    }
    return true;
}

static GuiManager *mGuiManager=NULL;

GuiManager *GuiManager::get() { return mGuiManager; }
void GuiManager::free() { if (mGuiManager) { delete mGuiManager; mGuiManager=NULL; } }
void GuiManager::init() {
    if (mGuiManager) throw Except(Except::ZIZI_INIT_ERR,"guimanager already exists");
    mGuiManager=new GuiManager();
}

GuiManager::GuiManager() : cursor(NULL), click(NULL), maingroup(new Group()) { }
GuiManager::~GuiManager() {
    unregister_self();
    delete maingroup;
    if (cursor) delete cursor;
    if (click)  delete click;
}

void GuiManager::register_self() {
    if (not cursor) {
        cursor = SpriteManager::get()->get_sprite("cursor");
        cursor->cx = cursor->w/2;
        cursor->cy = cursor->h/2;
        cursor->z = 3;
    }

    if (not click) click = SoundManager::get()->get_sfx("click");
}

void GuiManager::set_display(bool disp) {
    maingroup->enabled = disp;
}

void GuiManager::add_widget(Widget *widget,const std::string &name) {
    maingroup->add_widget(widget,name);
}

Widget *GuiManager::get_widget(const std::string &name) {
    return maingroup->get_widget(name);
}

bool GuiManager::key_down(SDLKey key) { return true; }

bool GuiManager::mouse_down(int button,float x,float y) {
    if (maingroup->interact(x,y)) click->play_once();
    return true;
}

bool GuiManager::frame_entered(float t,float dt) {
    SdlManager::get()->get_mouse_position(cursor->x,cursor->y);
    if (maingroup->draw(cursor->x,cursor->y,dt)) cursor->draw(dt);
    return true;
}

void GuiManager::unregister_self() { }

void toggle_music_callback(Button *but) {
    ToggleButton *casted = static_cast<ToggleButton*>(but);
    SoundManager::get()->set_playing_music(casted->state);
}

void toggle_sfx_callback(Button *but) {
    ToggleButton *casted = static_cast<ToggleButton*>(but);
    SoundManager::get()->set_playing_sfx(casted->state);
}

void GuiManager::add_sound_widgets(const std::string &grpname) {
    Group *sound_group = new Group();
    GuiManager::get()->add_widget(sound_group,grpname);

    Button *music_button = new ToggleButton("togglemusic",toggle_music_callback,SoundManager::get()->is_playing_music());
    music_button->sprite->x = SdlManager::get()->width - 20;
    music_button->sprite->y = 20;
    sound_group->add_widget(music_button,"music");
    Button *sfx_button = new ToggleButton("togglesfx",toggle_sfx_callback,SoundManager::get()->is_playing_sfx());
    sfx_button->sprite->x = music_button->sprite->x - 32;
    sfx_button->sprite->y = music_button->sprite->y;
    sound_group->add_widget(sfx_button,"sfx");
}
