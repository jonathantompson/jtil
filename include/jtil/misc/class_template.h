//
//  class_template.h
//
//  Created by Jonathan Tompson on 5/1/12.
//

#pragma once

namespace jtil {
namespace misc {

  class ClassTemplate {
  public:
    // Constructor / Destructor
    ClassTemplate();
    ~ClassTemplate();

  private:

    // Non-copyable, non-assignable.
    ClassTemplate(ClassTemplate&);
    ClassTemplate& operator=(const ClassTemplate&);
  };
};  // namespace misc
};  // namespace jtil
