#include "FlashText.hpp"
#include "GameFont.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace Paddle {
        FlashText::FlashText(GameFont &font, Vk::SwapChain& swapChain) 
                : font(font),
                  swapChain(swapChain) { }

        void FlashText::Flash(std::string text, float timeout) {
                messages.push_back(FlashMessage { text, time(NULL), timeout });
        }

        void FlashText::Update() {
                for(auto it = messages.begin(); it != messages.end(); ) {
                        if(difftime(time(NULL), it->time) > it->timeout)
                                it = messages.erase(it);
                        else ++it;
                }
        }

        void FlashText::Draw() {
                const float width  = static_cast<float>(swapChain.width());
                const float height = static_cast<float>(swapChain.height());
                const float scaleX = width  / 1080;
                const float scaleY = height / 720;

                for(size_t i = 0; i < messages.size(); ++i) {
                        auto msg = messages[i];
                        const float x    =  -50.0f  * scaleX;
                        const float y    = (-200.0f * scaleY) + i * 25;
                        const float size =  0.25f   * scaleX;

                        glm::vec3 color = glm::vec3(1.0f);
                        if(difftime(time(NULL), msg.time) > msg.timeout - 1)
                                color = glm::vec3(0.5f);

                        font.AddText(FontFamily::FONT_FAMILY_BODY, msg.text, x, y, size, color);
                }
        }
}
