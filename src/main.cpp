#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/loader/Event.hpp>
#include <Geode/ui/MDTextArea.hpp>
#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;

static std::string getDifficultyLabel(GJGameLevel* level) {
    int difficulty = (int)level->m_difficulty;
    bool isDemon = level->m_demon;
    bool isAuto  = level->m_autoLevel;

    if (isAuto) return "Auto";
    if (isDemon) {
        int demonDifficulty = level->m_demonDifficulty;
        switch (demonDifficulty) {
            case 3:  return "Easy Demon";
            case 4:  return "Medium Demon";
            case 6:  return "Insane Demon";
            case 7:  return "Extreme Demon";
            default: return "Hard Demon";
        }
    }
    switch (difficulty) {
        case 1:  return "Easy";
        case 2:  return "Normal";
        case 3:  return "Hard";
        case 4:  return "Harder";
        case 5:  return "Insane";
        default: return "NA";
    }
}

class $modify(MyPlayLayer, PlayLayer) {
    struct Fields {
        TaskHolder<web::WebResponse> m_webhookListener;
    };

    void levelComplete() {
        PlayLayer::levelComplete();
        
        if (this->m_isPracticeMode) {
            log::debug("[discord-webhook] Practice mode");
            return;
        }
        
        GJGameLevel* level = this->m_level;
        if (!level) {
            log::error("[discord-webhook] level is null");
            return;
        }
        
        auto webhookURL = Mod::get()->getSettingValue<std::string>("webhook-url");
        auto username   = Mod::get()->getSettingValue<std::string>("username");

        if (webhookURL.empty()) {
            log::warn("[discord-webhook] Webhook url is empty");
            return;
        }

        std::string levelName  = level->m_levelName;
        std::string difficulty = getDifficultyLabel(level);

        std::string jsonBody = R"({
    "username": ")" + username + R"(",
    "embeds": [{
        "title": "Level Completed!",
        "color": 5763719,
        "fields": [
            {
                "name": "Level",
                "value": ")" + levelName + R"(",
                "inline": true
            },
            {
                "name": "Difficulty",
                "value": ")" + difficulty + R"(",
                "inline": true
            }
        ],
        "footer": {
            "text": "Geometry Dash - Normal Mode"
        }
    }]
})";
        log::info("[discord-webhook] Normal mode level completed: {}", levelName);
        auto req = web::WebRequest();
        req.bodyString(jsonBody);
        req.header("Content-Type", "application/json");
        m_fields->m_webhookListener.spawn(
            "Discord Webhook",
            req.post(webhookURL),
            [this](web::WebResponse res) {
                if (res.ok()) {
                    log::info("[discord-webhook] Webhook sent successfully ({})", res.code());
                } else {
                    log::error("[discord-webhook] Webhook failed: HTTP {}", res.code());
                }
            }
        );
    }
};

class MyDropDown : public FLAlertLayer {
public:
    async::TaskHolder<web::WebResponse> m_request;

    MDTextArea* m_mdArea = nullptr;
    CCLabelBMFont* m_loadingLabel = nullptr;

    static MyDropDown* create() {
        auto ret = new MyDropDown();
        if (ret && ret->init(150)) {ret->autorelease(); return ret;}
        delete ret;
        return nullptr;
    }

    bool init(int opacity) {
        if (!FLAlertLayer::init(opacity)) return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();

        const float W = 400.f;
        const float H = 280.f;

        m_mainLayer->setContentSize({W, H});
        m_mainLayer->setPosition(winSize / 2);
        m_mainLayer->ignoreAnchorPointForPosition(false);




        auto cornerBL = CCSprite::createWithSpriteFrameName("dailyLevelCorner_001.png");
        cornerBL->setPosition({0.f, 0.f});
        cornerBL->setAnchorPoint({0.f, 0.f});
        m_mainLayer->addChild(cornerBL, 1);
        
        auto cornerBR = CCSprite::createWithSpriteFrameName("dailyLevelCorner_001.png");
        cornerBR->setPosition({W, 0.f});
        cornerBR->setAnchorPoint({1.f, 0.f});
        cornerBR->setFlipX(true);
        m_mainLayer->addChild(cornerBR, 1);

        auto cornerTL = CCSprite::createWithSpriteFrameName("dailyLevelCorner_001.png");
        cornerTL->setPosition({0.f, H});
        cornerTL->setAnchorPoint({0.f, 1.f});
        cornerTL->setFlipY(true);
        m_mainLayer->addChild(cornerTL, 1);

        auto cornerTR = CCSprite::createWithSpriteFrameName("dailyLevelCorner_001.png");
        cornerTR->setPosition({W, H});
        cornerTR->setAnchorPoint({1.f, 1.f});
        cornerTR->setFlipX(true);
        cornerTR->setFlipY(true);
        m_mainLayer->addChild(cornerTR, 1);

        auto bg = CCScale9Sprite::create("GJ_square04.png", {0, 0, 80, 80});
        bg->setContentSize({W, H});
        bg->setPosition({W / 2, H / 2});
        m_mainLayer->addChild(bg, -1);

        auto title = CCLabelBMFont::create("TUTORIAL", "phonk-hd.fnt"_spr);
        title->setScale(0.8f);
        title->setPosition({W / 2, H - 20.f});
        m_mainLayer->addChild(title, 1);

        auto menu = CCMenu::create();
        menu->setPosition({0, 0});

        auto closeBtn = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png"),
            this, menu_selector(MyDropDown::onClose)
        );
        closeBtn->setPosition({0.f, H - 10.f});
        menu->addChild(closeBtn);

        auto refreshBtnSprite = CCSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
        auto refreshBtn = CCMenuItemSpriteExtra::create(
            refreshBtnSprite,
            this, menu_selector(MyDropDown::onRefresh)
        );
        refreshBtn->setPosition({W, H - 10.f});
        menu->addChild(refreshBtn);

        
        
        m_mainLayer->addChild(menu, 3);

        m_loadingLabel = CCLabelBMFont::create("LOADING", "phonk-hd.fnt"_spr);
        m_loadingLabel->setScale(0.6f);
        m_loadingLabel->setPosition({W / 2, H / 2 - 10.f});
        m_loadingLabel->setVisible(false);
        m_mainLayer->addChild(m_loadingLabel);

        this->loadData();
        this->show();
        return true;

    }
    void onRefresh(CCObject*) {
        this->loadData();
    }

    void loadData() {
        const float W = 400.f;
        const float H = 280.f;
        const float mdWidth = W - 40.f;
        const float mdHeight = H - 60.f;
        m_request.cancel();

        if (m_mdArea) {
            m_mdArea->removeFromParent();
            m_mdArea = nullptr;
        }

        if (m_loadingLabel) {
            m_loadingLabel->setVisible(true);
        }

        auto req = web::WebRequest();

        std::string apiUrl = "https://5imcrj45k438p74gyfe7.c.websim.com/allfonts.md";

        m_request.spawn(
            req.get(apiUrl),
            [this, mdWidth, mdHeight, W, H](web::WebResponse res) {
                
                if (m_loadingLabel) {
                    m_loadingLabel->setVisible(false);
                }

                std::string markdownText;
                if (res.ok()) {
                    markdownText = res.string().unwrapOr("Erreur de format.");
                } else {
                    markdownText = fmt::format("# Erreur\nImpossible de charger les infos.\n\nCode HTTP : {}", res.code());
                }

                m_mdArea = MDTextArea::create(markdownText, {mdWidth, mdHeight});
                m_mdArea->setPosition({W / 2, H / 2 - 15.f});
                this->m_mainLayer->addChild(m_mdArea);
            }
        );
    }

    void onClose(CCObject*) {
        this->removeFromParentAndCleanup(true);
    }
};

class $modify(MyMenuLayer, MenuLayer) {
public:
    bool init() {
        if (!MenuLayer::init()) return false; 
        auto bottomMenu = this->getChildByID("bottom-menu");
        auto button = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("GJ_gkBtn_001.png"),
            this,
            menu_selector(MyMenuLayer::onButtonClick)
        );
        bottomMenu->addChild(button);
        return true;
    }

    void onButtonClick(CCObject*) {
        MyDropDown::create();
    }

    
};