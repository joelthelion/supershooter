#ifndef __GUIMANAGER_H__
#define __GUIMANAGER_H__

#include "engine.h"
#include "sound.h"
#include "sprite.h"

class Group; //forward declaration

class Widget {
public:
    Widget();
    virtual ~Widget();
    virtual bool interact(float x, float y) = 0;
    virtual void draw(float x,float y,float dt) const = 0;
    Group *get_root_group();

    bool enabled;
    Widget *parent;
};

class Group : public Widget {
public:
    Group();
    virtual ~Group();
    void add_widget(Widget *widget,const std::string &name);
    Widget *get_widget(const std::string &name);

    virtual bool interact(float x, float y);
    virtual void draw(float x,float y,float dt) const;

    typedef std::map<std::string,Widget*> Widgets;
    Widgets widgets;
};

class Button : public Widget {
public:
    Button(const std::string &sprname, void (*clicked)(Button*));
    virtual ~Button();

    virtual bool interact(float x, float y);
    virtual void draw(float x,float y,float dt) const;

    Sprite *sprite;
protected:
    bool is_click_valid(float x, float y);

    void (*clicked)(Button*);
};

class ToggleButton : public Button {
public:
    ToggleButton(const std::string &sprname, void (*toggled)(Button*), bool istate=false, Text *label=NULL);
    virtual ~ToggleButton();

    virtual bool interact(float x, float y);
    virtual void draw(float x,float y,float dt) const;

    bool state;
    Text *label;
private:
    StateSprite *casted;
};


class GuiManager : public Listener {
public:
    static GuiManager *get();
    static void free();
    static void init();

    void add_widget(Widget *widget,const std::string &name);
protected:
    GuiManager();
    ~GuiManager();

    virtual bool key_down(SDLKey key);
    virtual bool mouse_down(int button,float x,float y);
    virtual bool frame_entered(float t,float dt);
    virtual void register_self();
    virtual void unregister_self();

    Sprite *cursor;
    Sfx *click;
    Widget *mainwidget;
};

#endif