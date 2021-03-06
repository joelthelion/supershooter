#ifndef __SHOOT_H__
#define __SHOOT_H__

#include "engine.h"
#include "sprite.h"
#include "collision.h"
#include "sound.h"
#include "tinyxml.h"
#include <vector>
#include <set>
#include <stack>
#include <map>


//***********************************************************
struct Ship : public Area {
    Ship(float health,long int score);
    Ship(Sprite *body,float health,long int score);
    virtual ~Ship();
    virtual bool move(float dt)=0;
    virtual void draw(float dt) const;
    virtual float get_x() const;
    virtual float get_y() const;
    virtual float get_left() const;
    virtual float get_right() const;
    virtual float get_top() const;
    virtual float get_bottom() const;
    virtual bool collide_with(const Point *point) const;
    Sprite *body;
    float health;
    long int score;
};

class XmlShip : public Ship {
public:
    typedef std::map<std::string,Sprite*> Sprites;
    XmlShip(Sprite *aa,const Sprites &sprites,TiXmlElement *main,float health,long int score);

    virtual bool move(float dt);
protected:
    void exec();
    float speed,wait;
    float t;

    Sprites sprites;
    TiXmlElement *current;
    typedef std::stack<std::pair<TiXmlElement*,int> > ExecutionStack;
    ExecutionStack stack;
};

class ShipManager : public Listener {
public:
    static ShipManager *get();
    static void free();
    static void init(size_t nspace=2,const std::string &configfile="config.xml");

    void add_ship(Ship *ship,size_t kspace);
    void schedule_wave(const std::string &id);
    void flush_waves();
    void flush_ships();
    bool wave_finished() const;
    XmlShip *launch_enemy_ship(const std::string &id,const std::string &prgid,float x,float y,float angle);
    void dump(std::ostream &os=std::cout) const;
    long int score;
protected:
    ShipManager(size_t nspace,const std::string &configfile);
    ~ShipManager();

    virtual bool frame_entered(float t,float dt);
    virtual void unregister_self();

    typedef std::set<Ship*> Ships;
    typedef std::vector<Ships> Spaces;
    Spaces spaces;
    typedef std::multimap<float,Sprite*> Explosions;
    Explosions explosions;
    Sfx *boom;

    long int ncreated;
    long int ndestroyed;

    void xml_assert(bool v) const;
    Sprite *parse_sprite(TiXmlElement *node,XmlShip::Sprites &sprites,Sprite *parent=NULL) const;
    typedef std::map<std::string,TiXmlElement*> NodeMap;
    NodeMap shipnodes;
    NodeMap wavenodes;

    TiXmlDocument config;

    void exec();
    typedef std::deque<std::pair<TiXmlElement*,int> > ExecutionStack;
    ExecutionStack stack;
    TiXmlElement *current;
    float wait;
};

//***********************************************************
struct Bullet : public Point {
    Bullet(Sprite *sprite,float angle,float speed,float damage);
    virtual ~Bullet();
    virtual void move(float dt);
    virtual float get_x() const;
    virtual float get_y() const;
    Sprite *sprite;
    float vx,vy;
    float damage;
};

class BulletManager : public Listener {
public:
    static BulletManager *get();
    static void free();
    static void init(size_t nspace=2);

    Bullet *shoot(float x,float y,float angle, float speed, size_t kspace, const std::string &name="bullet00",float damage=2.);
    Bullet *shoot_from_sprite(const Sprite *sprite,float rangle, float speed, size_t kspace, const std::string &name="bullet00",float damage=2.);
    void flush_bullets();
protected:
    BulletManager(size_t nspace);
    ~BulletManager();

    virtual bool frame_entered(float t,float dt);
    virtual void unregister_self();
    void move(float dt);
    void draw(float dt) const;

    typedef std::set<Bullet*> Bullets;
    typedef std::vector<Bullets> Spaces;
    Spaces spaces;

    long int ncreated;
    long int ndestroyed;
};

#endif
