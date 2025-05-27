// Copyright 2025 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Test the behavior of a opt compile hint in a with scope for base std object to skip the
// lookup for with object.

(function TestBasicFunctionality() {
    var bar = {
      Object: 'override Object',
      Array: 'override Array'
    };
    
    with (bar) {
      //# @optNeverHideBaseStd
      assertNotEquals('override Object', Object.toString());
      assertTrue(Object.toString().includes('[native code]'));
    }
})();
  
(function TestWithoutAnnotation() {
  var bar = {
    Object: 'override Object'
  };
  
  with (bar) {
    assertEquals('override Object', Object);
  }
})();
  
(function TestMixedPropertyAccess() {
  var bar = {
    Object: 'override Object',
    custom: 'custom property'
  };
  
  with (bar) {
    //# @optNeverHideBaseStd
    assertNotEquals('override Object', Object.toString());
    assertEquals('custom property', custom);
  }
})();
  
// Nested with tests
(function TestNestedWithOuterAnnotated() {
  var outer = { Object: 'outer override' };
  var inner = { Object: 'inner override' };

  with (outer) {
    //# @optNeverHideBaseStd
    with (inner) {
      // Annotation affects outer with only
      assertNotEquals('outer override', Object.toString());
      assertEquals('inner override', Object);
    }
  }
})();

(function TestNestedWithInnerAnnotated() {
  var outer = { Object: 'outer override' };
  var inner = { Object: 'inner override' };

  // now in this situation, inner annotation will not work
  with (outer) {
    with (inner) {
      //# @optNeverHideBaseStd
      assertEquals('inner override', Object.toString());
    }
  }
})();

(function TestNestedWithBothAnnotated() {
  var outer = { Object: 'outer override' };
  var inner = { Object: 'inner override' };

  with (outer) {
    //# @optNeverHideBaseStd
    with (inner) {
      //# @optNeverHideBaseStd
      // Both annotations skip their respective with objects
      assertNotEquals('outer override', Object.toString());
      assertNotEquals('inner override', Object.toString());
      assertTrue(Object.toString().includes('[native code]'));
    }
  }
})();

(function TestNestedWithNeitherAnnotated() {
  var outer = { Object: 'outer override' };
  var inner = { Object: 'inner override' };

  with (outer) {
    with (inner) {
      // No annotations - innermost with takes precedence
      assertEquals('inner override', Object);
    }
  }
})();

// Supported base standard object tests
(function TestMultipleBaseStandardObjects() {
  var env = {
    Object: 'override Object',
    Array: 'override Array',
    Boolean: 'override Boolean',
    Symbol: 'override Symbol',
    String: 'override String',
    Number: 'override Number'
  };

  with (env) {
    //# @optNeverHideBaseStd
    assertTrue(Object.toString().includes('[native code]'));
    assertTrue(Array.toString().includes('[native code]'));
    assertTrue(Boolean.toString().includes('[native code]'));
    assertTrue(Symbol.toString().includes('[native code]'));
    assertTrue(Number.toString().includes('[native code]'));
    assertTrue(String.toString().includes('[native code]'));
  }
})();

(function TestAnnotationPositionSensitivity() {
  var bar = { Object: 'override' };

  with (bar) {
    // Annotation after other statements
    var x = 1;
    //# @optNeverHideBaseStd
    assertEquals('override', Object.toString());
  }

  with (bar) {
    // Annotation before other statements
    //# @optNeverHideBaseStd
    var y = 2;
    assertNotEquals('override', Object.toString());
  }
})();

(function TestWithBangFunction() {
  var scope = {
    Object: 'override Object',
    custom: 'custom property',
    func: function() { return 'scope function'; }
  };
  
  with (scope) {
    //# @optNeverHideBaseStd
    !function innerFunc() {
      assertNotEquals('override Object', Object.toString());     
      assertEquals('custom property', custom);
      assertEquals('scope function', func());
      var innerVar = 'inner variable';
      assertEquals('inner variable', innerVar);
    }();
    assertEquals('custom property', custom);
  }
})();

(function TestWithDirectStatement() {
  var bar = {
    Object: 'override Object',
    custom: 'custom property'
  };

  with(bar) //# @optNeverHideBaseStd
    assertNotEquals('[object Object]', Object.toString());
  
  with(bar) //# @optNeverHideBaseStd
    assertEquals('custom property', custom);
})();