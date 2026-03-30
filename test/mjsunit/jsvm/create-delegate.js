function testParamUnderlyingObject() {
  var proxy = new Proxy({}, {});
  var onlyChainProxy = { x: 1 };
  Object.setPrototypeOf(onlyChainProxy, proxy);
  var illegalInputs = [
    // Non-Object input will throw TypeError.
    undefined, null, 123, 3.14, 42n, 'foo', Symbol.toStringTag,
    // Unsupported exotic object (such as proxy) will throw TypeError.
    proxy, onlyChainProxy
  ];
  for (var underlying of illegalInputs) {
    assertThrows(() => {
      __Longque__.createDelegate(underlying);
    }, TypeError);
  }

  // If SKIP_PROTOTYPE_CHAIN, onlyChainProxy is valid underlyingObject.
  assertDoesNotThrow(() => {
    __Longque__.createDelegate(onlyChainProxy, undefined, __Longque__.SKIP_PROTOTYPE_CHAIN);
  });

  var validInputs = [
    {}, { foo: 'foo' }, [], ()=>{}, function() {}, globalThis,
    new Number(123), new String('foo'), Object(42n),
    Object, Function
  ];
  for (var underlying of validInputs) {
    assertDoesNotThrow(() => {
      __Longque__.createDelegate(underlying);
    });
  }
}

function testParamInitObject() {
  var proxy = new Proxy({}, {});
  var onlyChainProxy = { x: 1 };
  Object.setPrototypeOf(onlyChainProxy, proxy);
  const delegate = __Longque__.createDelegate({});
  var illegalInputs = [
    // Non-Object input will throw TypeError. undefined is ok because initObject is an optional parameter.
    null, 123, 3.14, 42n, 'foo', Symbol.toStringTag,
    proxy, delegate
  ];
  for (var initObject of illegalInputs) {
    assertThrows(() => {
      __Longque__.createDelegate({}, initObject);
    }, TypeError);
  }

  var validInputs = [
    undefined, {}, { foo: 'foo' }, [], ()=>{}, function() {}, globalThis,
    new Number(123), new String('foo'), Object(42n),
    Object, Function,
    onlyChainProxy
  ];
  for (var initObject of validInputs) {
    assertDoesNotThrow(() => {
      var delegate = __Longque__.createDelegate({}, initObject);
      if (initObject !== undefined) {
        assertTrue(delegate === initObject);
      }
    });
  }
}

function testParamPropertyFilterFlags() {
  var illegalInputs = [
    null, 'foo', Symbol.toStringTag, 3.14, {}, [], () => {}, globalThis, Object
  ];
  for (var filter of illegalInputs) {
    assertThrows(() => {
      __Longque__.createDelegate({}, undefined, filter);
    }, TypeError);
  }
}

function getUnderlyingObject() {
  var underlying = {
    x: 1,
    _y: 2,
    $z: 3,
    2008: 'Olympic',
    hello: function() { return 'hello'; },
    [Symbol.toStringTag]: function() { return 'u'; }
  };
  Object.defineProperty(underlying, 's', {
    writable: false,
    configurable: false,
    enumerable: false,
    value: 100
  });
  var proto = {
    _id: 4,
    name: 'hi'
  };
  Object.setPrototypeOf(underlying, proto);
  return underlying;
}

function getAllEnumerableStringPropertyKeys(object, skipPrototypeChain, filterFn) {
  var keys = [];
  if (skipPrototypeChain) {
     keys = Object.keys(object);
  } else {
    for (var key in object) {
      keys.push(key);
    }
  }
  if (typeof filterFn === 'function') {
    // Skip the name if filterFn(name) return true.
    keys = keys.filter(name => !filterFn(name));
  }
  return keys;
}

function assertStringArrayEqual(a1, a2) {
  assertTrue(Array.isArray(a1));
  assertTrue(Array.isArray(a2));
  assertEquals(a1.length, a2.length);
  for (var i = 0; i < a1.length; ++i) {
    assertEquals(a1[i], a2[i]);
  }
}

function testBasic() {
  var underlying = getUnderlyingObject();
  var delegate = __Longque__.createDelegate(underlying);
  for (var key in underlying) {
    assertEquals(delegate[key], underlying[key]);
    delegate[key] = Math.random();
  }
  for (var key in underlying) {
    assertEquals(delegate[key], underlying[key]);
    underlying[key] = Math.random();
  }
  for (var key in underlying) {
    assertEquals(delegate[key], underlying[key]);
  }
  // console.log(Object.keys(delegate));
  assertStringArrayEqual(
    Object.keys(delegate),
    getAllEnumerableStringPropertyKeys(underlying, false),
  );
}

function testFilter() {
  var underlying = getUnderlyingObject();
  var delegate = __Longque__.createDelegate(underlying, undefined, __Longque__.SKIP_PROTOTYPE_CHAIN);
  // console.log(Object.keys(delegate));
  assertStringArrayEqual(
    Object.keys(delegate),
    getAllEnumerableStringPropertyKeys(underlying, true)
  );

  var delegate = __Longque__.createDelegate(
    underlying,
    undefined,
    __Longque__.SKIP_PROTOTYPE_CHAIN |
      __Longque__.SKIP_PREFIX_UNDERSCORE |
      __Longque__.SKIP_PREFIX_DOLLAR,
  );
  // console.log(Object.keys(delegate));
  assertStringArrayEqual(
    Object.keys(delegate),
    getAllEnumerableStringPropertyKeys(underlying, true, (name) => {
      return name.startsWith("_") | name.startsWith("$");
    }),
  );
}

function testNonExtensibleInitObject() {
  var nonExtensibleObjectList = [
    Object.preventExtensions({}),
    Object.seal({}),
    Object.freeze({})
  ];
  for (var initObject of nonExtensibleObjectList) {
    assertTrue(!Object.isExtensible(initObject));
    assertThrows(() => {
      __Longque__.createDelegate({}, initObject);
    }, TypeError);
  }
}

function testOverridePropertyOfInitObject() {
  var underlying = {
    foo: 'foo',
    bar: 'bar',
  };
  var initObject = {
    x: 'x',
    foo: 'init_foo',  // will be override
  };
  var origFoo = initObject.foo;
  var delegate = __Longque__.createDelegate(underlying, initObject);
  assertSame(delegate, initObject);
  assertEquals(delegate.foo, underlying.foo);
  assertNotEquals(delegate.foo, origFoo);
}

function testRedefineNonConfigurablePropertyOfInitObject() {
  var underlying = { foo: () => { return 'foo'; } };
  var initObject = { 42: 1 };
  Object.defineProperty(initObject, 'foo', {
    value: 0,
    configurable: false,
    writable: true,
    enumerable: true
  });
  var result = undefined;
  assertThrows(() => {
    result = __Longque__.createDelegate(underlying, initObject);
  }, TypeError);
  assertTrue(result === undefined);
}

function testDelegateForAccessor() {
  var underlying = {
    x: 1,
    foo_: 10,
    get foo() {
      return 100;
    },
    set foo(val) {
      foo_ = val;
    },
  };
  var delegate = __Longque__.createDelegate(underlying);
  assertEquals(delegate.foo, underlying.foo);
  delegate.foo = 42;
  assertEquals(delegate.foo, underlying.foo);
  underlying.foo = 88;
  assertEquals(delegate.foo, underlying.foo);
}

function testDelegateForAccessorThatWillThrow() {
  var underlyingObject = {
    x: 1,
  };
  Object.defineProperty(underlyingObject, "foo", {
    enumerable: true,
    configurable: true,
    get: function () {
      throw new Error("foo get exception");
    },
    set: function () {
      throw new Error("foo set exception");
    },
  });

  var delegate = __Longque__.createDelegate(underlyingObject);
  assertThrows(() => { return delegate.foo; }, Error);
  assertThrows(() => { delegate.foo = 42; }, Error);
}

function testPropertyFilterFlagValue() {
  assertEquals(__Longque__.SKIP_PROTOTYPE_CHAIN, 1);
  assertEquals(__Longque__.SKIP_PREFIX_UNDERSCORE, 2);
  assertEquals(__Longque__.SKIP_PREFIX_DOLLAR, 4);
  assertEquals(__Longque__.SKIP_CONSTRUCTOR, 8);
}

function testDelegateChain() {
  const underlying = {
    x: 123
  };
  const d1 = __Longque__.createDelegate(underlying);
  assertTrue(d1.x === underlying.x);
  const d2 = __Longque__.createDelegate(d1);
  assertTrue(d2.x === underlying.x);
  const newVal = 42;
  d2.x = newVal;
  assertTrue(underlying.x === newVal);
  assertTrue(d1.x === newVal);
}

function testDelegateAsInitObject() {
  const underlying1 = {
    x: 123,
    y: 0
  };
  const underlying2 = {
    x: 456
  };

  const d1 = __Longque__.createDelegate(underlying1);
  assertTrue(d1.x === underlying1.x);
  // initObject should not be a delegate object
  assertThrows(() => {
    __Longque__.createDelegate(underlying2, d1);
  }, TypeError);

  const d2 = __Longque__.createDelegate({});
  // initObject should not be a delegate object
  assertThrows(() => {
    __Longque__.createDelegate(underlying2, d2);
  }, TypeError);
}

function testVersionConstancy() {
  assertTrue(typeof __Longque__.version === 'number');
  const version = __Longque__.version;
  __Longque__.version = Math.random();
  assertTrue(__Longque__.version === version);
}

function testLongqueConfigurable() {
  const longque = __Longque__;
  assertTrue(typeof longque === 'object');
  __Longque__ = 'other';
  // Can be configurable
  assertTrue(__Longque__ === 'other');
  __Longque__ = longque;
}

function testLongqueIsNotConstructor() {
  assertThrows(() => {
    new __Longque__();
  }, TypeError);
}

function longqueTest() {
  testVersionConstancy();
  testLongqueConfigurable();
  testLongqueIsNotConstructor();
}

function createDelegateTest() {
  testParamUnderlyingObject();
  testParamInitObject();
  testParamPropertyFilterFlags();
  testBasic();
  testFilter();
  testNonExtensibleInitObject();
  testOverridePropertyOfInitObject();
  testRedefineNonConfigurablePropertyOfInitObject();
  testDelegateForAccessor();
  testDelegateForAccessorThatWillThrow();
  testPropertyFilterFlagValue();
  testDelegateChain();
  testDelegateAsInitObject();
}

createDelegateTest();
longqueTest();
