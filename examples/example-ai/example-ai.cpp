
/*

                        /~` _  _ _|_. _     _ |_ | _
                        \_,(_)| | | || ||_|(_||_)|(/_

                    https://github.com/Naios/continuable
                                   v2.0.0

  Copyright(c) 2015 - 2018 Denis Blank <denis.blank at outlook dot com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files(the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions :

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
**/

#include <chrono>
#include <string>

#include "continuable/continuable.hpp"

using namespace std::chrono_literals;

struct Position {
  float x, y, z, o;
};

struct Unit {
  virtual ~Unit() = default;
};

struct UnitAI {
  virtual ~UnitAI() = default;

  virtual void OnEnterCombat() {
  }
};

struct CreatureAI : UnitAI {
  virtual cti::continuable<> MoveTo(Position pos) = 0;

  virtual cti::continuable<> CastSpell(unsigned id, Unit* target) = 0;

  virtual cti::continuable<> Say(std::string text,
                                 std::chrono::milliseconds duration) = 0;
};

struct MyCreatureAI : CreatureAI {
  void OnEnterCombat() override {
    CastSpell(3736, nullptr)
        .then(MoveTo({0, 0, 0, 0}) && Say("Walking...!", 5s))
        .then(Say("Done!", 5s))
        .fail(Say("Interrupted!", 5s));
  }
};

int main(int, char**) {

  // CreatureAI* ai = nullptr;
  // ai->OnEnterCombat();

  return 0;
}
