#include "shoot.h"
#include "except.h"
#include <cmath>
#include <iostream>
using std::cout;
using std::endl;

class StaticShip : public Ship {
public:
    StaticShip(Sprite *body,float ix,float iy) : Ship(body,100) {
        *x=ix;
        *y=iy;
    }
    bool move(float dt) { return true; }
};

class StaticArea : public Area {
public:
    StaticArea(Sprite *sprite) : Area(&sprite->x,&sprite->y), sprite(sprite) { w=sprite->w; h=sprite->h; }
    ~StaticArea() { delete sprite; }

    void draw() const { sprite->draw(); }
protected:
    Sprite *sprite;
};


class BigShip : public Ship, public Listener {
public:
    BigShip() : Ship(100), angle(M_PI), speed(0), shooting(false), reload(0) {
        body=SpriteManager::get()->get_sprite("bigship00");
        body->z=-1;
        //body->factorx=2.;
        //body->factory=2.;
        turrel_left=dynamic_cast<AnimatedSprite*>(body->create_child("turret00"));
        turrel_left->x=-16;
        turrel_left->y=-8;
        turrel_left->cx=10;
        turrel_left->z=1;
        turrel_left->angle=M_PI/180.*15;
        turrel_right=dynamic_cast<AnimatedSprite*>(body->create_child("turret00"));
        turrel_right->x=-16;
        turrel_right->y=8;
        turrel_right->cx=10;
        turrel_right->z=1;
        turrel_right->angle=-M_PI/180.*15;

        body->x=700;
        body->y=300;
        
        x=&body->x;
        y=&body->y;
        w=body->w;
        h=body->h;
    }

    virtual bool move(float dt) {
        *x+=dt*speed*cos(angle);
        *y+=dt*speed*sin(angle);
        body->angle=angle;

        if (reload>0) reload-=dt;

        if (shooting and reload<=0) {
            reload+=0.05;
            BulletManager::get()->shoot_from_sprite(turrel_left,0,300,0);
            BulletManager::get()->shoot_from_sprite(turrel_left,M_PI/180.*10.,300,0);
            BulletManager::get()->shoot_from_sprite(turrel_left,-M_PI/180.*10.,300,0);
            BulletManager::get()->shoot_from_sprite(turrel_right,0,300,0);
            BulletManager::get()->shoot_from_sprite(turrel_right,M_PI/180.*10.,300,0);
            BulletManager::get()->shoot_from_sprite(turrel_right,-M_PI/180.*10.,300,0);
        }

        if (shooting) { turrel_left->state=1; turrel_right->state=1; }
        else { turrel_left->state=0; turrel_right->state=0; }

        return true;
    }


protected:
    virtual bool key_down(SDLKey key) {
        switch (key) {
        case SDLK_SPACE:
            shooting=not shooting; break;
        }
        return true;
    }

    virtual bool frame_entered(float t,float dt) {
        const unsigned char *state=SdlManager::get()->get_key_state();
        if (state[SDLK_LEFT]) angle-=M_PI/180.*dt*180.;
        if (state[SDLK_RIGHT]) angle+=M_PI/180.*dt*180.;
        if (state[SDLK_UP]) speed+=dt*300.;
        if (state[SDLK_DOWN]) speed-=dt*300.;
        speed-=speed*1.*dt;

        float turrel_angle=M_PI/180.*(30.+20.*cos(2*M_PI*.8*t));
        turrel_left->angle=-turrel_angle;
        turrel_right->angle=turrel_angle;

        move(dt);
        draw();

        if (health<0) SdlManager::get()->unregister_listener(this);

        return true;
    }

    virtual void register_self() {
        CollisionManager::get()->spaces[1].second.insert(this);
    }

    virtual void unregister_self() {
        CollisionManager::get()->spaces[1].second.erase(this);
    }

    AnimatedSprite *turrel_left;
    AnimatedSprite *turrel_right;
    bool shooting;
    float angle,speed;
    float reload;
};
    
class Spawner : public Listener {
public:
    Spawner() {
        test=new StaticArea(SpriteManager::get()->get_sprite("aa"));
        *test->x=700;
        *test->y=300;
        CollisionManager::get()->spaces[1].second.insert(test);
    }
    ~Spawner() {
        CollisionManager::get()->spaces[1].second.erase(test);
        delete test;
    }
protected:
    virtual bool key_down(SDLKey key) {
        switch (key) {
        case SDLK_ESCAPE:
            return false; break;
        }
        return true;
    }

    virtual bool mouse_down(int button,float x,float y) {
        *test->x=x;
        *test->y=y;
        return true;
    }
    virtual bool frame_entered(float t,float dt) {
        test->draw();
        return true;
    }

    virtual void register_self() {
        ShipManager::get()->add_ship(new StaticShip(SpriteManager::get()->get_sprite("font"),30,500),0);
    }

    StaticArea *test;
};

#include "tinyxml/tinyxml.h"
#include <stack>

std::ostream &operator<<(std::ostream &os, const TiXmlElement *elem) {
    if (elem) {
        os<<"["<<elem->Value();
        TiXmlAttribute *att=const_cast<TiXmlElement*>(elem)->FirstAttribute();
        if (att) { cout<<"|"<<att->Name(); att=att->Next(); }
        while (att) { os<<" "<<att->Name(); att=att->Next(); }
        return os<<"]";
    }
    return os<<"[NULL]";
}

class XmlShip : public Ship {
public:
    typedef std::map<std::string,Sprite*> Sprites;
    XmlShip(Sprite *aa,const Sprites &sprites,TiXmlElement *main,float health,bool debug=false) : Ship(aa,health), angle(&aa->angle), sprites(sprites), current(main), t(0), speed(0), wait(0), debug(debug) {}

    virtual bool move(float dt) {
        if (*x>SdlManager::get()->width+256 or *x<-256 or *y>SdlManager::get()->height+256 or *y<-256) return false;

        *x+=dt*speed*cos(*angle);
        *y+=dt*speed*sin(*angle);

        if (debug) if (not stack.empty() or current) cout<<t<<": "<<current<<endl;

        exec();

        if (wait>0) wait-=dt;
        if (wait<0) wait=0;
        t+=dt;

        return true;
    }
    bool debug;
    float *angle;
protected:
    void exec() {
        if (wait>0) return;

        if (debug) for (size_t k=0; k<stack.size(); k++) cout<<"-";

        if (current) {
            if (current->ValueStr()=="program") {
                if (debug) cout<<"entering "<<current;
                int repeat=1;
                current->QueryValueAttribute("repeat",&repeat);
                if (debug) cout<<" repeat="<<repeat;
                stack.push(std::make_pair(current,repeat));
                if (debug) cout<<endl;
                current=current->FirstChildElement();
            } else if (current->ValueStr()=="wait") {
                if (debug) cout<<"entering "<<current;
                float time=1.;
                current->QueryValueAttribute("time",&time);
                if (debug) cout<<" time="<<time;
                wait=time;
                if (debug) cout<<endl;
                current=current->NextSiblingElement();
            } else if (current->ValueStr()=="position") {
                if (debug) cout<<"entering "<<current;

                std::string id;
                if (current->QueryValueAttribute("id",&id)==TIXML_SUCCESS) {
                    Sprites::iterator foo=sprites.find(id);
                    if (foo==sprites.end()) throw Except(Except::SS_XML_ID_UNKNOWN_ERR);
                    Sprite *select=foo->second;
                    current->QueryValueAttribute("x",&select->x);
                    current->QueryValueAttribute("y",&select->y);
                    if (current->QueryValueAttribute("angle",&select->angle)==TIXML_SUCCESS) select->angle*=M_PI/180.;
                    if (debug) cout<<" "<<id<<" "<<select->x<<" "<<select->y<<" "<<select->angle*180./M_PI<<endl;
                } else {
                    current->QueryValueAttribute("x",x);
                    current->QueryValueAttribute("y",y);
                    if (current->QueryValueAttribute("angle",angle)==TIXML_SUCCESS) *angle*=M_PI/180.;
                    if (debug) cout<<" "<<*x<<" "<<*y<<" "<<*angle*180./M_PI<<endl;
                }

                current=current->NextSiblingElement();
            } else if (current->ValueStr()=="positionrel") {
                if (debug) cout<<"entering "<<current;

                float _x=0,_y=0,_angle=0;
                current->QueryValueAttribute("x",&_x);
                current->QueryValueAttribute("y",&_y);
                if (current->QueryValueAttribute("angle",&_angle)==TIXML_SUCCESS) _angle*=M_PI/180.;

                std::string id;
                if (current->QueryValueAttribute("id",&id)==TIXML_SUCCESS) {
                    Sprites::iterator foo=sprites.find(id);
                    if (foo==sprites.end()) throw Except(Except::SS_XML_ID_UNKNOWN_ERR);
                    Sprite *select=foo->second;
                    select->x+=_x;
                    select->y+=_y;
                    select->angle+=_angle;
                    if (debug) cout<<" "<<id<<" "<<select->x<<" "<<select->y<<" "<<select->angle*180./M_PI<<endl;
                } else {
                    *x+=_x;
                    *y+=_y;
                    *angle+=_angle;
                    if (debug) cout<<" "<<*x<<" "<<*y<<" "<<*angle*180./M_PI<<endl;
                }

                current=current->NextSiblingElement();
            } else if (current->ValueStr()=="shoot") {
                if (debug) cout<<"entering "<<current;

                float _rangle=0,_speed=300.;
                current->QueryValueAttribute("speed",&_speed);
                if (current->QueryValueAttribute("anglerel",&_rangle)==TIXML_SUCCESS) _rangle*=M_PI/180.;

                std::string id;
                if (current->QueryValueAttribute("id",&id)==TIXML_SUCCESS) {
                    Sprites::iterator foo=sprites.find(id);
                    if (foo==sprites.end()) throw Except(Except::SS_XML_ID_UNKNOWN_ERR);
                    Sprite *select=foo->second;
                    BulletManager::get()->shoot_from_sprite(select,_rangle,_speed,1,"bullet01");
                } else BulletManager::get()->shoot_from_sprite(body,_rangle,_speed,1,"bullet01");

                if (debug) cout<<endl;
                current=current->NextSiblingElement();
            } else if (current->ValueStr()=="speed") {
                if (debug) cout<<"entering "<<current;
                current->QueryValueAttribute("value",&speed);
                if (debug) cout<<" "<<speed<<endl;
                current=current->NextSiblingElement();
            } else {
                if (debug) cout<<"unknow order "<<current<<endl;
                current=current->NextSiblingElement();
            }
        } else if (not stack.empty()) {
            ExecutionStack::value_type top=stack.top();
            stack.pop();

            top.second--;
            if (top.second) {
                if (debug) cout<<"repeating "<<top.first<<" "<<top.second<<endl;
                current=top.first->FirstChildElement();
                stack.push(top);
            } else {
                if (debug) cout<<"returning from "<<top.first<<endl;
                current=top.first->NextSiblingElement();
            }

        } 

        if (current or not stack.empty()) exec();
    }

    float speed,t,wait;

    Sprites sprites;
    TiXmlElement *current;
    typedef std::stack<std::pair<TiXmlElement*,int> > ExecutionStack;
    ExecutionStack stack;
};



class XmlEnemies {
public:
    XmlEnemies(const std::string &configfile) {
        xml_assert(config.LoadFile(configfile));

        TiXmlElement *root=config.FirstChildElement("config");
        xml_assert(root);

        for (TiXmlElement *ships=root->FirstChildElement("ships"); ships; ships=ships->NextSiblingElement("ships"))
        for (TiXmlElement *ship=ships->FirstChildElement("ship"); ship; ship=ship->NextSiblingElement("ship")) {
            std::string id;
            xml_assert(ship->QueryValueAttribute("id",&id)==TIXML_SUCCESS);
            if (shipnodes.find(id)!=shipnodes.end()) throw Except(Except::SS_XML_ID_DUPLICATE_ERR);
            shipnodes[id]=ship;
        }

    }

    void const dump(std::ostream &os=std::cout) {
        os<<shipnodes.size()<<" ships"<<endl;
    }

    XmlShip *launch_ship(const std::string &id,const std::string &prgid,float x,float y,float angle) {
        ShipNodes::iterator foo=shipnodes.find(id);
        if (foo==shipnodes.end()) throw Except(Except::SS_XML_ID_UNKNOWN_ERR);

        float health;
        xml_assert(foo->second->QueryValueAttribute("health",&health)==TIXML_SUCCESS);

        XmlShip::Sprites sprites;
        Sprite *body=parse_sprite(foo->second->FirstChildElement("sprite"),sprites);

        TiXmlElement *program=NULL;
        for (TiXmlElement *i=foo->second->FirstChildElement("program"); i; i=i->NextSiblingElement("program")) {
            std::string current_prgid;
            i->QueryValueAttribute("id",&current_prgid);
            if (prgid==current_prgid) { program=i; break; }
        }
        if (not program) throw Except(Except::SS_XML_ID_UNKNOWN_ERR);

        XmlShip *ship=new XmlShip(body,sprites,program,health);
        *ship->x=x;
        *ship->y=y;
        *ship->angle=angle;
        ShipManager::get()->add_ship(ship,0); //add as enemy
        return ship;
    }

    Sprite *parse_sprite(TiXmlElement *node,XmlShip::Sprites &sprites,Sprite *parent=NULL) const {
        std::string name;
        xml_assert(node->QueryValueAttribute("name",&name)==TIXML_SUCCESS);

        Sprite *body;
        if (parent) body=parent->create_child(name);
        else body=SpriteManager::get()->get_sprite(name);

        std::string id;
        node->QueryValueAttribute("id",&id);
        if (not id.empty()) {
            if (sprites.find(id)!=sprites.end()) throw Except(Except::SS_XML_ID_DUPLICATE_ERR);
            sprites[id]=body;
        }

        node->QueryValueAttribute("z",&body->z);
        node->QueryValueAttribute("x",&body->x);
        node->QueryValueAttribute("y",&body->y);
        node->QueryValueAttribute("cx",&body->cx);
        node->QueryValueAttribute("cy",&body->cy);
        if (node->QueryValueAttribute("angle",&body->angle)==TIXML_SUCCESS) body->angle*=M_PI/180.;

        for (TiXmlElement *child=node->FirstChildElement("sprite"); child; child=child->NextSiblingElement("sprite"))
            parse_sprite(child,sprites,body);

        if (parent) return NULL;
        return body;
    }

protected:
    void xml_assert(bool v) const { if (not v) throw Except(Except::SS_XML_PARSING_ERR,config.ErrorDesc()); }
    typedef std::map<std::string,TiXmlElement*> ShipNodes;
    ShipNodes shipnodes;

    TiXmlDocument config;
};





int main() {
   // try {
        SdlManager::init();
        SpriteManager::init();
        CollisionManager::init();
        BulletManager::init();
        ShipManager::init();

        SpriteManager::get()->load_directory("data");
        SpriteManager::get()->dump();

        SdlManager::get()->register_listener(BulletManager::get());
        SdlManager::get()->register_listener(ShipManager::get());
        SdlManager::get()->set_background_color(.5,.6,.7);
        {
            XmlEnemies aaa("config.xml");
            aaa.dump();
            aaa.launch_ship("bigship","main",0,300,0);
            aaa.launch_ship("bigship","main",0,400,0);
            for (float y=100; y<=600; y+=50) aaa.launch_ship("basicship","main",0,y,0);
            for (float y=100; y<=600; y+=50) aaa.launch_ship("basicship","main",-50,y,0);

            //Spawner spawner;
            BigShip bigship;
            //SdlManager::get()->register_listener(&spawner);
            SdlManager::get()->register_listener(&bigship);
            SdlManager::get()->main_loop();
        }
        ShipManager::free();
        BulletManager::free();
        CollisionManager::free();
        SpriteManager::free();
        SdlManager::free();
   // } catch (Except e) {
   //     e.dump();
   //     return 1;
   // }
}


