#include "shoot.h"

#include <cmath>
#include "collision.h"
#include "except.h"
#include <iostream>
#include <algorithm>
#include <typeinfo>
using std::cout;
using std::endl;

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

//***********************************************************
Ship::Ship(float health) : body(NULL), health(health) {}
Ship::Ship(Sprite *body,float health) : body(body), health(health) {}
Ship::~Ship() { delete body; }
void Ship::draw(float dt) const { body->draw(dt); }
float Ship::get_x() const { return body->x; }
float Ship::get_y() const { return body->y; }
float Ship::get_left() const { 
    float dx0=(body->w*body->factorx*cos(body->angle)+body->h*body->factory*sin(body->angle))/2.;
    float dx1=(body->w*body->factorx*cos(body->angle)-body->h*body->factory*sin(body->angle))/2.;
    if (dx0<0) dx0=-dx0;
    if (dx1<0) dx1=-dx1;
    return dx0<dx1 ? body->x-dx1 : body->x-dx0;
}
float Ship::get_right() const { 
    float dx0=(body->w*body->factorx*cos(body->angle)-body->h*body->factory*sin(body->angle))/2.;
    float dx1=(body->w*body->factorx*cos(body->angle)+body->h*body->factory*sin(body->angle))/2.;
    if (dx0<0) dx0=-dx0;
    if (dx1<0) dx1=-dx1;
    return dx0<dx1 ? body->x+dx1 : body->x+dx0;
}
float Ship::get_top() const {
    float dy0=(body->w*body->factorx*sin(body->angle)+body->h*body->factory*cos(body->angle))/2.;
    float dy1=(-body->w*body->factorx*sin(body->angle)+body->h*body->factory*cos(body->angle))/2.;
    if (dy0<0) dy0=-dy0;
    if (dy1<0) dy1=-dy1;
    return dy0<dy1 ? body->y-dy1 : body->y-dy0;
}
float Ship::get_bottom() const {
    float dy0=(body->w*body->factorx*sin(body->angle)+body->h*body->factory*cos(body->angle))/2.;
    float dy1=(-body->w*body->factorx*sin(body->angle)+body->h*body->factory*cos(body->angle))/2.;
    if (dy0<0) dy0=-dy0;
    if (dy1<0) dy1=-dy1;
    return dy0<dy1 ? body->y+dy1 : body->y+dy0;
}
bool Ship::collide_with(const Point *point) const {
    float dx=(point->get_x()-body->x);
    float dy=(point->get_y()-body->y);
    float lx=(dx*cos(body->angle)+dy*sin(body->angle))*2./body->factorx;
    if (lx<0) lx=-lx;
    if (lx>body->w) return false;
    float ly=(-dx*sin(body->angle)+dy*cos(body->angle))*2./body->factory;
    if (ly<0) ly=-ly;
    if (ly>body->h) return false;
    return true;
}

XmlShip::XmlShip(Sprite *aa,const Sprites &sprites,TiXmlElement *main,float health,bool debug) : Ship(aa,health), sprites(sprites), current(main), t(0), speed(0), wait(0), debug(debug) {}

bool XmlShip::move(float dt) {
    if (body->x>SdlManager::get()->width+256 or body->x<-256 or body->y>SdlManager::get()->height+256 or body->y<-256) return false;

    body->x+=dt*speed*cos(body->angle);
    body->y+=dt*speed*sin(body->angle);

    if (debug) if (not stack.empty() or current) cout<<t<<": "<<current<<endl;

    exec();

    if (wait>0) wait-=dt;
    if (wait<0) wait=0;
    t+=dt;

    return true;
}

void XmlShip::exec() {
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
                if (foo==sprites.end()) throw Except(Except::SS_XML_ID_UNKNOWN_ERR,id);
                Sprite *select=foo->second;
                current->QueryValueAttribute("x",&select->x);
                current->QueryValueAttribute("y",&select->y);
                if (current->QueryValueAttribute("angle",&select->angle)==TIXML_SUCCESS) select->angle*=M_PI/180.;
                if (debug) cout<<" "<<id<<" "<<select->x<<" "<<select->y<<" "<<select->angle*180./M_PI<<endl;
            } else {
                current->QueryValueAttribute("x",&body->x);
                current->QueryValueAttribute("y",&body->y);
                if (current->QueryValueAttribute("angle",&body->angle)==TIXML_SUCCESS) body->angle*=M_PI/180.;
                if (debug) body->dump();
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
                if (foo==sprites.end()) throw Except(Except::SS_XML_ID_UNKNOWN_ERR,id);
                Sprite *select=foo->second;
                select->x+=_x;
                select->y+=_y;
                select->angle+=_angle;
                if (debug) cout<<" "<<id<<" "<<select->x<<" "<<select->y<<" "<<select->angle*180./M_PI<<endl;
            } else {
                body->x+=_x;
                body->y+=_y;
                body->angle+=_angle;
                if (debug) body->dump();
            }

            current=current->NextSiblingElement();
        } else if (current->ValueStr()=="shoot") {
            if (debug) cout<<"entering "<<current;

            float _rangle=0,_speed=300.;
            std::string _name="bullet01";
            current->QueryValueAttribute("speed",&_speed);
            current->QueryValueAttribute("name",&_name);
            if (current->QueryValueAttribute("anglerel",&_rangle)==TIXML_SUCCESS) _rangle*=M_PI/180.;

            std::string id;
            Bullet *bullet;
            if (current->QueryValueAttribute("id",&id)==TIXML_SUCCESS) {
                Sprites::iterator foo=sprites.find(id);
                if (foo==sprites.end()) throw Except(Except::SS_XML_ID_UNKNOWN_ERR,id);
                Sprite *select=foo->second;
                bullet=BulletManager::get()->shoot_from_sprite(select,_rangle,_speed,1,_name);
            } else bullet=BulletManager::get()->shoot_from_sprite(body,_rangle,_speed,1,_name);

            if (StateSprite *cast=dynamic_cast<StateSprite*>(bullet->sprite)) current->QueryValueAttribute("sprstate",&cast->state);
            if (AnimatedSprite *cast=dynamic_cast<AnimatedSprite*>(bullet->sprite)) {
                current->QueryValueAttribute("sprspeed",&cast->speed);
                current->QueryValueAttribute("sprrepeat",&cast->repeat);
                current->QueryValueAttribute("sprlength",&cast->length);
            }

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

//***********************************************************
static ShipManager *mShipManager=NULL;

ShipManager *ShipManager::get() { return mShipManager; }
void ShipManager::free() { if (mShipManager) { delete mShipManager; mShipManager=NULL; } }
void ShipManager::init(size_t nspace,const std::string &configfile) {
    if (mShipManager) throw Except(Except::SS_INIT_ERR);
    mShipManager=new ShipManager(nspace,configfile);
}

ShipManager::ShipManager(size_t nspace,const std::string &configfile) : spaces(nspace), ncreated(0), ndestroyed(0) {
    xml_assert(config.LoadFile(configfile));

    TiXmlElement *root=config.FirstChildElement("config");
    xml_assert(root);

    for (TiXmlElement *ships=root->FirstChildElement("ships"); ships; ships=ships->NextSiblingElement("ships"))
    for (TiXmlElement *ship=ships->FirstChildElement("ship"); ship; ship=ship->NextSiblingElement("ship")) {
        std::string id;
        xml_assert(ship->QueryValueAttribute("id",&id)==TIXML_SUCCESS);
        if (shipnodes.find(id)!=shipnodes.end()) throw Except(Except::SS_XML_ID_DUPLICATE_ERR,id);
        shipnodes[id]=ship;
    }

}

ShipManager::~ShipManager() {
    unregister_self();
    cout<<ncreated<<" ships created, "<<ndestroyed<<" ships destroyed: ";
    if (ncreated==ndestroyed) cout<<"no leak detected"<<endl;
    else cout<<"leak detected"<<endl;
}

void ShipManager::unregister_self() {
    size_t kspace=0;
    for (Spaces::iterator ships=spaces.begin(); ships!=spaces.end(); ships++) {
        for (Ships::const_iterator i=ships->begin(); i!=ships->end(); i++) {
            CollisionManager::get()->spaces[kspace].second.erase(*i);
            delete *i;
        }
        ndestroyed+=ships->size();
        ships->clear();
        kspace++;
    }
}

bool ShipManager::frame_entered(float t,float dt) {

    size_t kspace=0;
    for (Spaces::iterator ships=spaces.begin(); ships!=spaces.end(); ships++) {
        for (Ships::iterator i=ships->begin(); i!=ships->end(); i++) {
            if ((*i)->health<0 or not (*i)->move(dt)) {
                delete *i;
                CollisionManager::get()->spaces[kspace].second.erase(*i);
                ships->erase(i);
                ndestroyed++;
                continue;
            }
            (*i)->draw(dt);
        }
        kspace++;
    }

    return true;
}

void ShipManager::add_ship(Ship *ship,size_t kspace) {
    spaces[kspace].insert(ship);
    CollisionManager::get()->spaces[kspace].second.insert(ship);
    
    ncreated++;
}

XmlShip *ShipManager::launch_enemy_ship(const std::string &id,const std::string &prgid,float x,float y,float angle) {
    ShipNodes::iterator foo=shipnodes.find(id);
    if (foo==shipnodes.end()) throw Except(Except::SS_XML_ID_UNKNOWN_ERR,id);

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
    if (not program) throw Except(Except::SS_XML_ID_UNKNOWN_ERR,prgid);

    XmlShip *ship=new XmlShip(body,sprites,program,health);
    ship->body->x=x;
    ship->body->y=y;
    ship->body->angle=angle;
    add_ship(ship,0); //add as enemy
    return ship;
}

Sprite *ShipManager::parse_sprite(TiXmlElement *node,XmlShip::Sprites &sprites,Sprite *parent) const {
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
    if (StateSprite *cast=dynamic_cast<StateSprite*>(body)) node->QueryValueAttribute("state",&cast->state);
    if (AnimatedSprite *cast=dynamic_cast<AnimatedSprite*>(body)) {
        node->QueryValueAttribute("speed",&cast->speed);
        node->QueryValueAttribute("repeat",&cast->repeat);
        node->QueryValueAttribute("length",&cast->length);
    }

    for (TiXmlElement *child=node->FirstChildElement("sprite"); child; child=child->NextSiblingElement("sprite"))
        parse_sprite(child,sprites,body);

    if (parent) return NULL;
    return body;
}

void ShipManager::dump(std::ostream &os) const {
    os<<shipnodes.size()<<" ships"<<endl;
}

void ShipManager::xml_assert(bool v) const { if (not v) throw Except(Except::SS_XML_PARSING_ERR,config.ErrorDesc()); }

//***********************************************************
Bullet::Bullet(Sprite *sprite,float angle,float speed,float damage) : sprite(sprite), vx(speed*cos(angle)), vy(speed*sin(angle)), damage(damage) { sprite->angle=angle;}
Bullet::~Bullet() { delete sprite; }
float Bullet::get_x() const { return sprite->x; }
float Bullet::get_y() const { return sprite->y; }
void Bullet::move(float dt) { sprite->x+=dt*vx; sprite->y+=dt*vy; }

static BulletManager *mBulletManager=NULL;

BulletManager *BulletManager::get() { return mBulletManager; }
void BulletManager::free() { if (mBulletManager) { delete mBulletManager; mBulletManager=NULL; } }
void BulletManager::init(size_t nspace) {
    if (mBulletManager) throw Except(Except::SS_INIT_ERR);
    mBulletManager=new BulletManager(nspace);
}

BulletManager::BulletManager(size_t nspace) : spaces(nspace), ncreated(0), ndestroyed(0) {}
BulletManager::~BulletManager() {
    unregister_self();
    cout<<ncreated<<" bullets created, "<<ndestroyed<<" bullets destroyed: ";
    if (ncreated==ndestroyed) cout<<"no leak detected"<<endl;
    else cout<<"leak detected"<<endl;
}

void BulletManager::unregister_self() {
    size_t kspace=0;
    for (Spaces::iterator bullets=spaces.begin(); bullets!=spaces.end(); bullets++) {
        for (Bullets::const_iterator i=bullets->begin(); i!=bullets->end(); i++) {
            CollisionManager::get()->spaces[kspace].first.erase(*i);
            delete *i;
        }
        ndestroyed+=bullets->size();
        bullets->clear();
        kspace++;
    }
}

bool BulletManager::frame_entered(float t,float dt) {
    move(dt);

    CollisionManager::get()->resolve_collision();
    //CollisionManager::get()->dump();
    for (size_t k=0; k<spaces.size(); k++) {
        CollisionManager::Space &space=CollisionManager::get()->spaces[k];
        Bullets &bullets=spaces[k];

        Area::Points foo;
        for (CollisionManager::Areas::const_iterator i=space.second.begin(); i!=space.second.end(); i++) {
            const Area *area=*i;
            std::set_union(area->colliding.begin(),area->colliding.end(),foo.begin(),foo.end(),std::inserter(foo,foo.begin()));
            //cout<<(dynamic_cast<const Ship*>(area)!=NULL)<<" "<<typeid(*area).before(typeid(Ship))<<endl;
            if (const Ship *ship=dynamic_cast<const Ship*>(area)) { //area is a ship so deal the damage //FIXME use of typeid is time constant
                for (Area::Points::const_iterator j=ship->colliding.begin(); j!=ship->colliding.end(); j++) {
                    if (const Bullet *bullet=dynamic_cast<const Bullet*>(*j)) {
                        const_cast<Ship*>(ship)->health-=bullet->damage;
                    }
                }
            }


        }
        //cout<<"---------"<<endl;

        for (Area::Points::const_iterator j=foo.begin(); j!=foo.end(); j++) {
           space.first.erase(*j);
           bullets.erase(dynamic_cast<Bullet*>(*j));
           delete *j; 
           ndestroyed++;
        }
    }

    draw(dt);
    return true;
}

Bullet *BulletManager::shoot(float x,float y,float angle,float speed,size_t kspace,const std::string &name,float damage) {
    Sprite *sprite=SpriteManager::get()->get_sprite(name);
    sprite->x=x;
    sprite->y=y;
    Bullet *bullet=new Bullet(sprite,angle,speed,damage);
    spaces[kspace].insert(bullet);
    CollisionManager::get()->spaces[kspace].first.insert(bullet);
    ncreated++;
    return bullet;
}

Bullet *BulletManager::shoot_from_sprite(const Sprite *sprite,float rangle,float speed,size_t kspace,const std::string &name,float damage) {
    float ax,ay,aangle,afactorx,afactory;
    sprite->absolute_coordinates(ax,ay,aangle,afactorx,afactory);
    return shoot(ax,ay,aangle+rangle,speed,kspace,name,damage);
}

void BulletManager::move(float dt) { 
    size_t kspace=0;
    for (Spaces::iterator j=spaces.begin(); j!=spaces.end(); j++) {
        Bullets &bullets=*j;
        for (Bullets::iterator i=bullets.begin(); i!=bullets.end(); i++) {
            Bullet *bullet=*i;
            bullet->move(dt);
            if (bullet->sprite->x<-20 or bullet->sprite->x>SdlManager::get()->width+20 or bullet->sprite->y<-20 or bullet->sprite->y>SdlManager::get()->height+20) {
                delete bullet;
                bullets.erase(i);
                CollisionManager::get()->spaces[kspace].first.erase(bullet);
                ndestroyed++;
            }
        }
        kspace++;
    }
}

void BulletManager::draw(float dt) const {
    for (Spaces::const_iterator bullets=spaces.begin(); bullets!=spaces.end(); bullets++)
    for (Bullets::const_iterator i=bullets->begin(); i!=bullets->end(); i++)
        (*i)->sprite->draw(dt);
}

