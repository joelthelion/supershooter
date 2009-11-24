#include "except.h"
#include "utils.h"
#include "message.h"
#include "sound.h"
#include <cassert>
using std::cerr;
using std::cout;
using std::endl;

struct Pixel {
    Pixel(float x,float y,unsigned int state) : left(NULL), right(NULL), top(NULL), bottom(NULL) {
        sprite = dynamic_cast<StateSprite*>(SpriteManager::get()->get_sprite("blocks"));
        assert(sprite);
        sprite->x = x;
        sprite->y = y;
        assert(state < sprite->nstate);
        sprite->state = state;
    }
    ~Pixel() {
        delete sprite;
    }
    Pixel *left;
    Pixel *right;
    Pixel *top;
    Pixel *bottom;
    StateSprite *sprite;
};

class MainApp : public Listener {
public:
    MainApp() : nw(10), nh(5), size(nw*nh), spacing(35) { 
        cursor=SpriteManager::get()->get_sprite("cursor");
        cursor->cx=cursor->w/2.;
        cursor->cy=cursor->h/2.;
        cursor->z=2.;

        pixels = new Pixel*[size];
        const float cx = (SdlManager::get()->width-spacing*(nw-1))/2.;
        const float cy = (SdlManager::get()->height-spacing*(nh-1))/2.;
        for (int i=0; i<nh; i++) for (int j=0; j<nw; j++) {
            get_pixel(i,j) = new Pixel(cx+spacing*j,cy+spacing*i,i%4);
        }
        for (int i=0; i<nh; i++) for (int j=0; j<nw; j++) {
            Pixel *current = get_pixel(i,j);
            if (i != 0)    current->top    = get_pixel(i-1,j);
            if (i != nh-1) current->bottom = get_pixel(i+1,j);
            if (j != 0)    current->left   = get_pixel(i,j-1);
            if (j != nw-1) current->right  = get_pixel(i,j+1);
        }
        for (int k=0; k<size; k++) {
            Pixel *current = pixels[k];
            int n = 0;
            if (current->left) n++;
            if (current->right) n++;
            if (current->top) n++;
            if (current->bottom) n++;
            current->sprite->state = n;
        }
    }
    ~MainApp() {
        unregister_self();
        for (int k=0; k<size; k++) { delete pixels[k]; }
        delete pixels;
    }
protected:
    Pixel*& get_pixel(int i,int j) { return pixels[i*nw+j]; }
    virtual bool key_down(SDLKey key) {
        switch (key) {
        case SDLK_ESCAPE:
            return false; break;
        default:
            return true; break;
        }
    }
    virtual bool mouse_down(int button,float x,float y) {
        return true;
    }
    virtual bool mouse_up(int button,float x,float y) {
        return true;
    }
    virtual bool frame_entered(float t,float dt) {
        SdlManager::get()->get_mouse_position(cursor->x,cursor->y);
        cursor->draw(dt);

        for (int k=0; k<size; k++) { pixels[k]->sprite->draw(dt); }
        return true;
    }
    virtual void unregister_self() {
    }
    const int nw;
    const int nh;
    const int size;
    const float spacing;
    Pixel **pixels;
    Sprite *cursor;
};

int main() {
    try {
        SdlManager::init();
        SdlManager::get()->set_background_color(0,0,0);

        SoundManager::init();
        if (not SoundManager::get()->load_directory("data"))
        if (not SoundManager::get()->load_directory("../data"))
        if (not SoundManager::get()->load_directory("../../data")) {
            cerr<<"can't locate sound data..."<<endl;
            return 1;
        }
        SoundManager::get()->dump();

        SpriteManager::init();
        if (not SpriteManager::get()->load_directory("data"))
        if (not SpriteManager::get()->load_directory("../data"))
        if (not SpriteManager::get()->load_directory("../../data")) {
            cerr<<"can't locate sprite data..."<<endl;
            return 1;
        }
        SpriteManager::get()->dump();

        MessageManager::init();
        SdlManager::get()->register_listener(MessageManager::get());
        MessageManager::get()->set_display(true);
        {
            Fps fps;
            fps.set_display(true);
            MainApp mainapp;
            SdlManager::get()->register_listener(&fps);
            SdlManager::get()->register_listener(&mainapp);

            MessageManager::get()->add_message("lets get started");
            SdlManager::get()->main_loop();
        }
        SoundManager::free();
        MessageManager::free();
        SpriteManager::free();
        SdlManager::free();
    } catch (Except e) {
        e.dump();
        return 1;
    }
}

