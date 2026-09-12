// dist/esm/ts/errors/external-error.js
var ExternalError = class extends Error {
  /** @internal */
  constructor(type, message) {
    super(message);
    this.type = type;
  }
};
var ConfigurationError = class extends ExternalError {
  /** @internal */
  constructor(message, node) {
    super("Configuration", message);
    this.node = node;
  }
};
var PopoutBlockedError = class extends ExternalError {
  /** @internal */
  constructor(message) {
    super("PopoutBlocked", message);
  }
};
var ApiError = class extends ExternalError {
  /** @internal */
  constructor(message) {
    super("API", message);
  }
};
var BindError = class extends ExternalError {
  /** @internal */
  constructor(message) {
    super("Bind", message);
  }
};

// dist/esm/ts/errors/internal-error.js
var InternalError = class extends Error {
  constructor(type, code, message) {
    super(`${type}: ${code}${message === void 0 ? "" : ": " + message}`);
  }
};
var AssertError = class extends InternalError {
  constructor(code, message) {
    super("Assert", code, message);
  }
};
var UnreachableCaseError = class extends InternalError {
  constructor(code, variableValue, message) {
    super("UnreachableCase", code, `${variableValue}${message === void 0 ? "" : ": " + message}`);
  }
};
var UnexpectedNullError = class extends InternalError {
  constructor(code, message) {
    super("UnexpectedNull", code, message);
  }
};
var UnexpectedUndefinedError = class extends InternalError {
  constructor(code, message) {
    super("UnexpectedUndefined", code, message);
  }
};

// dist/esm/ts/utils/i18n-strings.js
var I18nStrings;
(function(I18nStrings2) {
  let initialised = false;
  const infosObject = {
    PopoutCannotBeCreatedWithGroundItemConfig: {
      id: 0,
      default: "Popout cannot be created with ground ItemConfig"
    },
    PleaseRegisterAConstructorFunction: {
      id: 1,
      default: "Please register a constructor function"
    },
    ComponentTypeNotRegisteredAndBindComponentEventHandlerNotAssigned: {
      id: 2,
      default: "Component type not registered and BindComponentEvent handler not assigned"
    },
    ComponentIsAlreadyRegistered: {
      id: 3,
      default: "Component is already registered"
    },
    ComponentIsNotVirtuable: {
      id: 4,
      default: "Component is not virtuable. Requires rootHtmlElement field/getter"
    },
    VirtualComponentDoesNotHaveRootHtmlElement: {
      id: 5,
      default: 'Virtual component does not have getter "rootHtmlElement"'
    },
    ItemConfigIsNotTypeComponent: {
      id: 6,
      default: "ItemConfig is not of type component"
    },
    InvalidNumberPartInSizeString: {
      id: 7,
      default: "Invalid number part in size string"
    },
    UnknownUnitInSizeString: {
      id: 8,
      default: "Unknown unit in size string"
    },
    UnsupportedUnitInSizeString: {
      id: 9,
      default: "Unsupported unit in size string"
    }
  };
  I18nStrings2.idCount = Object.keys(infosObject).length;
  const infos = Object.values(infosObject);
  function checkInitialise() {
    if (!initialised) {
      for (let i = 0; i < I18nStrings2.idCount; i++) {
        const info = infos[i];
        if (info.id !== i) {
          throw new AssertError("INSI00110", `${i}: ${info.id}`);
        } else {
          i18nStrings[i] = info.default;
        }
      }
    }
    initialised = true;
  }
  I18nStrings2.checkInitialise = checkInitialise;
})(I18nStrings || (I18nStrings = {}));
var i18nStrings = new Array(I18nStrings.idCount);

// dist/esm/ts/utils/style-constants.js
var StyleConstants;
(function(StyleConstants2) {
  StyleConstants2.defaultComponentBaseZIndex = "auto";
  StyleConstants2.defaultComponentDragZIndex = "32";
  StyleConstants2.defaultComponentStackMaximisedZIndex = "41";
})(StyleConstants || (StyleConstants = {}));

// dist/esm/ts/utils/types.js
var WidthOrHeightPropertyName;
(function(WidthOrHeightPropertyName2) {
  WidthOrHeightPropertyName2.width = "width";
  WidthOrHeightPropertyName2.height = "height";
})(WidthOrHeightPropertyName || (WidthOrHeightPropertyName = {}));
var Side;
(function(Side2) {
  Side2.top = "top";
  Side2.left = "left";
  Side2.right = "right";
  Side2.bottom = "bottom";
})(Side || (Side = {}));
var LogicalZIndex;
(function(LogicalZIndex2) {
  LogicalZIndex2.base = "base";
  LogicalZIndex2.drag = "drag";
  LogicalZIndex2.stackMaximised = "stackMaximised";
})(LogicalZIndex || (LogicalZIndex = {}));
var LogicalZIndexToDefaultMap = {
  base: StyleConstants.defaultComponentBaseZIndex,
  drag: StyleConstants.defaultComponentDragZIndex,
  stackMaximised: StyleConstants.defaultComponentStackMaximisedZIndex
};
var JsonValue;
(function(JsonValue2) {
  function isJson(value) {
    return isJsonObject(value);
  }
  JsonValue2.isJson = isJson;
  function isJsonObject(value) {
    return !Array.isArray(value) && value !== null && typeof value === "object";
  }
  JsonValue2.isJsonObject = isJsonObject;
})(JsonValue || (JsonValue = {}));
var ItemType;
(function(ItemType2) {
  ItemType2.ground = "ground";
  ItemType2.row = "row";
  ItemType2.column = "column";
  ItemType2.stack = "stack";
  ItemType2.component = "component";
})(ItemType || (ItemType = {}));
var ResponsiveMode;
(function(ResponsiveMode2) {
  ResponsiveMode2.none = "none";
  ResponsiveMode2.always = "always";
  ResponsiveMode2.onload = "onload";
})(ResponsiveMode || (ResponsiveMode = {}));
var SizeUnitEnum;
(function(SizeUnitEnum2) {
  SizeUnitEnum2["Pixel"] = "px";
  SizeUnitEnum2["Percent"] = "%";
  SizeUnitEnum2["Fractional"] = "fr";
  SizeUnitEnum2["Em"] = "em";
})(SizeUnitEnum || (SizeUnitEnum = {}));
(function(SizeUnitEnum2) {
  function tryParse(value) {
    switch (value) {
      case SizeUnitEnum2.Pixel:
        return SizeUnitEnum2.Pixel;
      case SizeUnitEnum2.Percent:
        return SizeUnitEnum2.Percent;
      case SizeUnitEnum2.Fractional:
        return SizeUnitEnum2.Fractional;
      case SizeUnitEnum2.Em:
        return SizeUnitEnum2.Em;
      default:
        return void 0;
    }
  }
  SizeUnitEnum2.tryParse = tryParse;
  function format(value) {
    switch (value) {
      case SizeUnitEnum2.Pixel:
        return SizeUnitEnum2.Pixel;
      case SizeUnitEnum2.Percent:
        return SizeUnitEnum2.Percent;
      case SizeUnitEnum2.Fractional:
        return SizeUnitEnum2.Fractional;
      case SizeUnitEnum2.Em:
        return SizeUnitEnum2.Em;
      default:
        throw new UnreachableCaseError("SUEF44998", value);
    }
  }
  SizeUnitEnum2.format = format;
})(SizeUnitEnum || (SizeUnitEnum = {}));

// dist/esm/ts/utils/utils.js
function numberToPixels(value) {
  return value.toString(10) + "px";
}
function pixelsToNumber(value) {
  const numberStr = value.replace("px", "");
  return parseFloat(numberStr);
}
function splitStringAtFirstNonNumericChar(value) {
  value = value.trimStart();
  const length = value.length;
  if (length === 0) {
    return { numericPart: "", firstNonNumericCharPart: "" };
  } else {
    let firstNonDigitPartIndex = length;
    let gotDecimalPoint = false;
    for (let i = 0; i < length; i++) {
      const char = value[i];
      if (!isDigit(char)) {
        if (char !== ".") {
          firstNonDigitPartIndex = i;
          break;
        } else {
          if (gotDecimalPoint) {
            firstNonDigitPartIndex = i;
            break;
          } else {
            gotDecimalPoint = true;
          }
        }
      }
    }
    const digitsPart = value.substring(0, firstNonDigitPartIndex);
    const firstNonDigitPart = value.substring(firstNonDigitPartIndex).trim();
    return { numericPart: digitsPart, firstNonNumericCharPart: firstNonDigitPart };
  }
}
function isDigit(char) {
  return char >= "0" && char <= "9";
}
function getElementWidth(element) {
  return element.offsetWidth;
}
function setElementWidth(element, width) {
  const widthAsPixels = numberToPixels(width);
  element.style.width = widthAsPixels;
}
function getElementHeight(element) {
  return element.offsetHeight;
}
function setElementHeight(element, height) {
  const heightAsPixels = numberToPixels(height);
  element.style.height = heightAsPixels;
}
function getElementWidthAndHeight(element) {
  return {
    width: element.offsetWidth,
    height: element.offsetHeight
  };
}
function setElementDisplayVisibility(element, visible) {
  if (visible) {
    element.style.display = "";
  } else {
    element.style.display = "none";
  }
}
function ensureElementPositionAbsolute(element) {
  const absolutePosition = "absolute";
  if (element.style.position !== absolutePosition) {
    element.style.position = absolutePosition;
  }
}
function deepExtend(target, obj) {
  if (obj !== void 0) {
    for (const key in obj) {
      if (obj.hasOwnProperty(key)) {
        const value = obj[key];
        const existingTarget = target[key];
        target[key] = deepExtendValue(existingTarget, value);
      }
    }
  }
  return target;
}
function deepExtendValue(existingTarget, value) {
  if (typeof value !== "object") {
    return value;
  } else {
    if (Array.isArray(value)) {
      const length = value.length;
      const targetArray = new Array(length);
      for (let i = 0; i < length; i++) {
        const element = value[i];
        targetArray[i] = deepExtendValue({}, element);
      }
      return targetArray;
    } else {
      if (value === null) {
        return null;
      } else {
        const valueObj = value;
        if (existingTarget === void 0) {
          return deepExtend({}, valueObj);
        } else {
          if (typeof existingTarget !== "object") {
            return deepExtend({}, valueObj);
          } else {
            if (Array.isArray(existingTarget)) {
              return deepExtend({}, valueObj);
            } else {
              if (existingTarget === null) {
                return deepExtend({}, valueObj);
              } else {
                const existingTargetObj = existingTarget;
                return deepExtend(existingTargetObj, valueObj);
              }
            }
          }
        }
      }
    }
  }
}
function removeFromArray(item, array) {
  const index = array.indexOf(item);
  if (index === -1) {
    throw new Error("Can't remove item from array. Item is not in the array");
  }
  array.splice(index, 1);
}
function getUniqueId() {
  return (Math.random() * 1e15).toString(36).replace(".", "");
}
function getErrorMessage(e) {
  if (e instanceof Error) {
    return e.message;
  } else {
    if (typeof e === "string") {
      return e;
    } else {
      return "Unknown Error";
    }
  }
}

// dist/esm/ts/utils/config-minifier.js
var ConfigMinifier;
(function(ConfigMinifier2) {
  const keys = [
    "settings",
    "hasHeaders",
    "constrainDragToContainer",
    "selectionEnabled",
    "dimensions",
    "borderWidth",
    "minItemHeight",
    "minItemWidth",
    "headerHeight",
    "dragProxyWidth",
    "dragProxyHeight",
    "labels",
    "close",
    "maximise",
    "minimise",
    "popout",
    "content",
    "componentType",
    "componentState",
    "id",
    "width",
    "type",
    "height",
    "isClosable",
    "title",
    "popoutWholeStack",
    "openPopouts",
    "parentId",
    "activeItemIndex",
    "reorderEnabled",
    "borderGrabWidth"
    //Maximum 36 entries, do not cross this line!
  ];
  const values = [
    true,
    false,
    "row",
    "column",
    "stack",
    "component",
    "close",
    "maximise",
    "minimise",
    "open in new window"
  ];
  function checkInitialise() {
    if (keys.length > 36) {
      throw new Error("Too many keys in config minifier map");
    }
  }
  ConfigMinifier2.checkInitialise = checkInitialise;
  function translateObject(from, minify) {
    const to = {};
    for (const key in from) {
      if (from.hasOwnProperty(key)) {
        let translatedKey;
        if (minify) {
          translatedKey = minifyKey(key);
        } else {
          translatedKey = unminifyKey(key);
        }
        const fromValue = from[key];
        to[translatedKey] = translateValue(fromValue, minify);
      }
    }
    return to;
  }
  ConfigMinifier2.translateObject = translateObject;
  function translateArray(from, minify) {
    const length = from.length;
    const to = new Array(length);
    for (let i = 0; i < length; i++) {
      const fromValue = from[i];
      to[i] = translateValue(fromValue, minify);
    }
    return to;
  }
  function translateValue(from, minify) {
    if (typeof from === "object") {
      if (from === null) {
        return null;
      } else {
        if (Array.isArray(from)) {
          return translateArray(from, minify);
        } else {
          return translateObject(from, minify);
        }
      }
    } else {
      if (minify) {
        return minifyValue(from);
      } else {
        return unminifyValue(from);
      }
    }
  }
  function minifyKey(value) {
    if (typeof value === "string" && value.length === 1) {
      return "___" + value;
    }
    const index = indexOfKey(value);
    if (index === -1) {
      return value;
    } else {
      return index.toString(36);
    }
  }
  function unminifyKey(key) {
    if (key.length === 1) {
      return keys[parseInt(key, 36)];
    }
    if (key.substr(0, 3) === "___") {
      return key[3];
    }
    return key;
  }
  function minifyValue(value) {
    if (typeof value === "string" && value.length === 1) {
      return "___" + value;
    }
    const index = indexOfValue(value);
    if (index === -1) {
      return value;
    } else {
      return index.toString(36);
    }
  }
  function unminifyValue(value) {
    if (typeof value === "string" && value.length === 1) {
      return values[parseInt(value, 36)];
    }
    if (typeof value === "string" && value.substr(0, 3) === "___") {
      return value[3];
    }
    return value;
  }
  function indexOfKey(key) {
    for (let i = 0; i < keys.length; i++) {
      if (keys[i] === key) {
        return i;
      }
    }
    return -1;
  }
  function indexOfValue(value) {
    for (let i = 0; i < values.length; i++) {
      if (values[i] === value) {
        return i;
      }
    }
    return -1;
  }
})(ConfigMinifier || (ConfigMinifier = {}));

// dist/esm/ts/config/resolved-config.js
var ResolvedItemConfig;
(function(ResolvedItemConfig2) {
  ResolvedItemConfig2.defaults = {
    type: ItemType.ground,
    content: [],
    size: 1,
    sizeUnit: SizeUnitEnum.Fractional,
    minSize: void 0,
    minSizeUnit: SizeUnitEnum.Pixel,
    id: "",
    isClosable: true
  };
  function createCopy(original, content) {
    switch (original.type) {
      case ItemType.ground:
      case ItemType.row:
      case ItemType.column:
        return ResolvedRowOrColumnItemConfig.createCopy(original, content);
      case ItemType.stack:
        return ResolvedStackItemConfig.createCopy(original, content);
      case ItemType.component:
        return ResolvedComponentItemConfig.createCopy(original);
      default:
        throw new UnreachableCaseError("CICC91354", original.type, "Invalid Config Item type specified");
    }
  }
  ResolvedItemConfig2.createCopy = createCopy;
  function createDefault(type) {
    switch (type) {
      case ItemType.ground:
        throw new AssertError("CICCDR91562");
      // Get default root from LayoutConfig
      case ItemType.row:
      case ItemType.column:
        return ResolvedRowOrColumnItemConfig.createDefault(type);
      case ItemType.stack:
        return ResolvedStackItemConfig.createDefault();
      case ItemType.component:
        return ResolvedComponentItemConfig.createDefault();
      default:
        throw new UnreachableCaseError("CICCDD91563", type, "Invalid Config Item type specified");
    }
  }
  ResolvedItemConfig2.createDefault = createDefault;
  function isComponentItem(itemConfig) {
    return itemConfig.type === ItemType.component;
  }
  ResolvedItemConfig2.isComponentItem = isComponentItem;
  function isStackItem(itemConfig) {
    return itemConfig.type === ItemType.stack;
  }
  ResolvedItemConfig2.isStackItem = isStackItem;
  function isGroundItem(itemConfig) {
    return itemConfig.type === ItemType.ground;
  }
  ResolvedItemConfig2.isGroundItem = isGroundItem;
})(ResolvedItemConfig || (ResolvedItemConfig = {}));
var ResolvedHeaderedItemConfig;
(function(ResolvedHeaderedItemConfig2) {
  ResolvedHeaderedItemConfig2.defaultMaximised = false;
  let Header2;
  (function(Header3) {
    function createCopy(original, show) {
      if (original === void 0) {
        return void 0;
      } else {
        return {
          show: show !== null && show !== void 0 ? show : original.show,
          popout: original.popout,
          close: original.close,
          maximise: original.maximise,
          minimise: original.minimise,
          tabDropdown: original.tabDropdown
        };
      }
    }
    Header3.createCopy = createCopy;
  })(Header2 = ResolvedHeaderedItemConfig2.Header || (ResolvedHeaderedItemConfig2.Header = {}));
})(ResolvedHeaderedItemConfig || (ResolvedHeaderedItemConfig = {}));
var ResolvedStackItemConfig;
(function(ResolvedStackItemConfig2) {
  ResolvedStackItemConfig2.defaultActiveItemIndex = 0;
  function createCopy(original, content) {
    const result = {
      type: original.type,
      content: content !== void 0 ? copyContent(content) : copyContent(original.content),
      size: original.size,
      sizeUnit: original.sizeUnit,
      minSize: original.minSize,
      minSizeUnit: original.minSizeUnit,
      id: original.id,
      maximised: original.maximised,
      isClosable: original.isClosable,
      activeItemIndex: original.activeItemIndex,
      header: ResolvedHeaderedItemConfig.Header.createCopy(original.header)
    };
    return result;
  }
  ResolvedStackItemConfig2.createCopy = createCopy;
  function copyContent(original) {
    const count = original.length;
    const result = new Array(count);
    for (let i = 0; i < count; i++) {
      result[i] = ResolvedItemConfig.createCopy(original[i]);
    }
    return result;
  }
  ResolvedStackItemConfig2.copyContent = copyContent;
  function createDefault() {
    const result = {
      type: ItemType.stack,
      content: [],
      size: ResolvedItemConfig.defaults.size,
      sizeUnit: ResolvedItemConfig.defaults.sizeUnit,
      minSize: ResolvedItemConfig.defaults.minSize,
      minSizeUnit: ResolvedItemConfig.defaults.minSizeUnit,
      id: ResolvedItemConfig.defaults.id,
      maximised: ResolvedHeaderedItemConfig.defaultMaximised,
      isClosable: ResolvedItemConfig.defaults.isClosable,
      activeItemIndex: ResolvedStackItemConfig2.defaultActiveItemIndex,
      header: void 0
    };
    return result;
  }
  ResolvedStackItemConfig2.createDefault = createDefault;
})(ResolvedStackItemConfig || (ResolvedStackItemConfig = {}));
var ResolvedComponentItemConfig;
(function(ResolvedComponentItemConfig2) {
  ResolvedComponentItemConfig2.defaultReorderEnabled = true;
  function resolveComponentTypeName(itemConfig) {
    const componentType = itemConfig.componentType;
    if (typeof componentType === "string") {
      return componentType;
    } else {
      return void 0;
    }
  }
  ResolvedComponentItemConfig2.resolveComponentTypeName = resolveComponentTypeName;
  function createCopy(original) {
    const result = {
      type: original.type,
      content: [],
      size: original.size,
      sizeUnit: original.sizeUnit,
      minSize: original.minSize,
      minSizeUnit: original.minSizeUnit,
      id: original.id,
      maximised: original.maximised,
      isClosable: original.isClosable,
      reorderEnabled: original.reorderEnabled,
      title: original.title,
      header: ResolvedHeaderedItemConfig.Header.createCopy(original.header),
      componentType: original.componentType,
      componentState: deepExtendValue(void 0, original.componentState)
    };
    return result;
  }
  ResolvedComponentItemConfig2.createCopy = createCopy;
  function createDefault(componentType = "", componentState, title = "") {
    const result = {
      type: ItemType.component,
      content: [],
      size: ResolvedItemConfig.defaults.size,
      sizeUnit: ResolvedItemConfig.defaults.sizeUnit,
      minSize: ResolvedItemConfig.defaults.minSize,
      minSizeUnit: ResolvedItemConfig.defaults.minSizeUnit,
      id: ResolvedItemConfig.defaults.id,
      maximised: ResolvedHeaderedItemConfig.defaultMaximised,
      isClosable: ResolvedItemConfig.defaults.isClosable,
      reorderEnabled: ResolvedComponentItemConfig2.defaultReorderEnabled,
      title,
      header: void 0,
      componentType,
      componentState
    };
    return result;
  }
  ResolvedComponentItemConfig2.createDefault = createDefault;
  function copyComponentType(componentType) {
    return deepExtendValue({}, componentType);
  }
  ResolvedComponentItemConfig2.copyComponentType = copyComponentType;
})(ResolvedComponentItemConfig || (ResolvedComponentItemConfig = {}));
var ResolvedRowOrColumnItemConfig;
(function(ResolvedRowOrColumnItemConfig2) {
  function isChildItemConfig(itemConfig) {
    switch (itemConfig.type) {
      case ItemType.row:
      case ItemType.column:
      case ItemType.stack:
      case ItemType.component:
        return true;
      case ItemType.ground:
        return false;
      default:
        throw new UnreachableCaseError("CROCOSPCICIC13687", itemConfig.type);
    }
  }
  ResolvedRowOrColumnItemConfig2.isChildItemConfig = isChildItemConfig;
  function createCopy(original, content) {
    const result = {
      type: original.type,
      content: content !== void 0 ? copyContent(content) : copyContent(original.content),
      size: original.size,
      sizeUnit: original.sizeUnit,
      minSize: original.minSize,
      minSizeUnit: original.minSizeUnit,
      id: original.id,
      isClosable: original.isClosable
    };
    return result;
  }
  ResolvedRowOrColumnItemConfig2.createCopy = createCopy;
  function copyContent(original) {
    const count = original.length;
    const result = new Array(count);
    for (let i = 0; i < count; i++) {
      result[i] = ResolvedItemConfig.createCopy(original[i]);
    }
    return result;
  }
  ResolvedRowOrColumnItemConfig2.copyContent = copyContent;
  function createDefault(type) {
    const result = {
      type,
      content: [],
      size: ResolvedItemConfig.defaults.size,
      sizeUnit: ResolvedItemConfig.defaults.sizeUnit,
      minSize: ResolvedItemConfig.defaults.minSize,
      minSizeUnit: ResolvedItemConfig.defaults.minSizeUnit,
      id: ResolvedItemConfig.defaults.id,
      isClosable: ResolvedItemConfig.defaults.isClosable
    };
    return result;
  }
  ResolvedRowOrColumnItemConfig2.createDefault = createDefault;
})(ResolvedRowOrColumnItemConfig || (ResolvedRowOrColumnItemConfig = {}));
var ResolvedRootItemConfig;
(function(ResolvedRootItemConfig2) {
  function createCopy(config) {
    return ResolvedItemConfig.createCopy(config);
  }
  ResolvedRootItemConfig2.createCopy = createCopy;
  function isRootItemConfig(itemConfig) {
    switch (itemConfig.type) {
      case ItemType.row:
      case ItemType.column:
      case ItemType.stack:
      case ItemType.component:
        return true;
      case ItemType.ground:
        return false;
      default:
        throw new UnreachableCaseError("CROCOSPCICIC13687", itemConfig.type);
    }
  }
  ResolvedRootItemConfig2.isRootItemConfig = isRootItemConfig;
})(ResolvedRootItemConfig || (ResolvedRootItemConfig = {}));
var ResolvedGroundItemConfig;
(function(ResolvedGroundItemConfig2) {
  function create(rootItemConfig) {
    const content = rootItemConfig === void 0 ? [] : [rootItemConfig];
    return {
      type: ItemType.ground,
      content,
      size: 100,
      sizeUnit: SizeUnitEnum.Percent,
      minSize: 0,
      minSizeUnit: SizeUnitEnum.Pixel,
      id: "",
      isClosable: false,
      title: "",
      reorderEnabled: false
    };
  }
  ResolvedGroundItemConfig2.create = create;
})(ResolvedGroundItemConfig || (ResolvedGroundItemConfig = {}));
var ResolvedLayoutConfig;
(function(ResolvedLayoutConfig2) {
  let Settings;
  (function(Settings2) {
    Settings2.defaults = {
      constrainDragToContainer: true,
      reorderEnabled: true,
      popoutWholeStack: false,
      blockedPopoutsThrowError: true,
      closePopoutsOnUnload: true,
      responsiveMode: ResponsiveMode.none,
      tabOverlapAllowance: 0,
      reorderOnTabMenuClick: true,
      tabControlOffset: 10,
      popInOnClose: false
    };
    function createCopy2(original) {
      return {
        constrainDragToContainer: original.constrainDragToContainer,
        reorderEnabled: original.reorderEnabled,
        popoutWholeStack: original.popoutWholeStack,
        blockedPopoutsThrowError: original.blockedPopoutsThrowError,
        closePopoutsOnUnload: original.closePopoutsOnUnload,
        responsiveMode: original.responsiveMode,
        tabOverlapAllowance: original.tabOverlapAllowance,
        reorderOnTabMenuClick: original.reorderOnTabMenuClick,
        tabControlOffset: original.tabControlOffset,
        popInOnClose: original.popInOnClose
      };
    }
    Settings2.createCopy = createCopy2;
  })(Settings = ResolvedLayoutConfig2.Settings || (ResolvedLayoutConfig2.Settings = {}));
  let Dimensions;
  (function(Dimensions2) {
    function createCopy2(original) {
      return {
        borderWidth: original.borderWidth,
        borderGrabWidth: original.borderGrabWidth,
        defaultMinItemHeight: original.defaultMinItemHeight,
        defaultMinItemHeightUnit: original.defaultMinItemHeightUnit,
        defaultMinItemWidth: original.defaultMinItemWidth,
        defaultMinItemWidthUnit: original.defaultMinItemWidthUnit,
        headerHeight: original.headerHeight,
        dragProxyWidth: original.dragProxyWidth,
        dragProxyHeight: original.dragProxyHeight
      };
    }
    Dimensions2.createCopy = createCopy2;
    Dimensions2.defaults = {
      borderWidth: 5,
      borderGrabWidth: 5,
      defaultMinItemHeight: 0,
      defaultMinItemHeightUnit: SizeUnitEnum.Pixel,
      defaultMinItemWidth: 10,
      defaultMinItemWidthUnit: SizeUnitEnum.Pixel,
      headerHeight: 20,
      dragProxyWidth: 300,
      dragProxyHeight: 200
    };
  })(Dimensions = ResolvedLayoutConfig2.Dimensions || (ResolvedLayoutConfig2.Dimensions = {}));
  let Header2;
  (function(Header3) {
    function createCopy2(original) {
      return {
        show: original.show,
        popout: original.popout,
        dock: original.dock,
        close: original.close,
        maximise: original.maximise,
        minimise: original.minimise,
        tabDropdown: original.tabDropdown
      };
    }
    Header3.createCopy = createCopy2;
    Header3.defaults = {
      show: Side.top,
      popout: "open in new window",
      dock: "dock",
      maximise: "maximise",
      minimise: "minimise",
      close: "close",
      tabDropdown: "additional tabs"
    };
  })(Header2 = ResolvedLayoutConfig2.Header || (ResolvedLayoutConfig2.Header = {}));
  function isPopout(config) {
    return "parentId" in config;
  }
  ResolvedLayoutConfig2.isPopout = isPopout;
  function createDefault() {
    const result = {
      root: void 0,
      openPopouts: [],
      dimensions: ResolvedLayoutConfig2.Dimensions.defaults,
      settings: ResolvedLayoutConfig2.Settings.defaults,
      header: ResolvedLayoutConfig2.Header.defaults,
      resolved: true
    };
    return result;
  }
  ResolvedLayoutConfig2.createDefault = createDefault;
  function createCopy(config) {
    if (isPopout(config)) {
      return ResolvedPopoutLayoutConfig.createCopy(config);
    } else {
      const result = {
        root: config.root === void 0 ? void 0 : ResolvedRootItemConfig.createCopy(config.root),
        openPopouts: ResolvedLayoutConfig2.copyOpenPopouts(config.openPopouts),
        settings: ResolvedLayoutConfig2.Settings.createCopy(config.settings),
        dimensions: ResolvedLayoutConfig2.Dimensions.createCopy(config.dimensions),
        header: ResolvedLayoutConfig2.Header.createCopy(config.header),
        resolved: config.resolved
      };
      return result;
    }
  }
  ResolvedLayoutConfig2.createCopy = createCopy;
  function copyOpenPopouts(original) {
    const count = original.length;
    const result = new Array(count);
    for (let i = 0; i < count; i++) {
      result[i] = ResolvedPopoutLayoutConfig.createCopy(original[i]);
    }
    return result;
  }
  ResolvedLayoutConfig2.copyOpenPopouts = copyOpenPopouts;
  function minifyConfig(layoutConfig) {
    return ConfigMinifier.translateObject(layoutConfig, true);
  }
  ResolvedLayoutConfig2.minifyConfig = minifyConfig;
  function unminifyConfig(minifiedConfig) {
    return ConfigMinifier.translateObject(minifiedConfig, false);
  }
  ResolvedLayoutConfig2.unminifyConfig = unminifyConfig;
})(ResolvedLayoutConfig || (ResolvedLayoutConfig = {}));
var ResolvedPopoutLayoutConfig;
(function(ResolvedPopoutLayoutConfig2) {
  let Window;
  (function(Window2) {
    function createCopy2(original) {
      return {
        width: original.width,
        height: original.height,
        left: original.left,
        top: original.top
      };
    }
    Window2.createCopy = createCopy2;
    Window2.defaults = {
      width: null,
      height: null,
      left: null,
      top: null
    };
  })(Window = ResolvedPopoutLayoutConfig2.Window || (ResolvedPopoutLayoutConfig2.Window = {}));
  function createCopy(original) {
    const result = {
      root: original.root === void 0 ? void 0 : ResolvedRootItemConfig.createCopy(original.root),
      openPopouts: ResolvedLayoutConfig.copyOpenPopouts(original.openPopouts),
      settings: ResolvedLayoutConfig.Settings.createCopy(original.settings),
      dimensions: ResolvedLayoutConfig.Dimensions.createCopy(original.dimensions),
      header: ResolvedLayoutConfig.Header.createCopy(original.header),
      parentId: original.parentId,
      indexInParent: original.indexInParent,
      window: ResolvedPopoutLayoutConfig2.Window.createCopy(original.window),
      resolved: original.resolved
    };
    return result;
  }
  ResolvedPopoutLayoutConfig2.createCopy = createCopy;
})(ResolvedPopoutLayoutConfig || (ResolvedPopoutLayoutConfig = {}));

// dist/esm/ts/config/config.js
var ItemConfig;
(function(ItemConfig2) {
  function resolve(itemConfig, rowAndColumnChildLegacySizeDefault) {
    switch (itemConfig.type) {
      case ItemType.ground:
        throw new ConfigurationError("ItemConfig cannot specify type ground", JSON.stringify(itemConfig));
      case ItemType.row:
      case ItemType.column:
        return RowOrColumnItemConfig.resolve(itemConfig, rowAndColumnChildLegacySizeDefault);
      case ItemType.stack:
        return StackItemConfig.resolve(itemConfig, rowAndColumnChildLegacySizeDefault);
      case ItemType.component:
        return ComponentItemConfig.resolve(itemConfig, rowAndColumnChildLegacySizeDefault);
      default:
        throw new UnreachableCaseError("UCUICR55499", itemConfig.type);
    }
  }
  ItemConfig2.resolve = resolve;
  function resolveContent(content) {
    if (content === void 0) {
      return [];
    } else {
      const count = content.length;
      const result = new Array(count);
      for (let i = 0; i < count; i++) {
        result[i] = ItemConfig2.resolve(content[i], false);
      }
      return result;
    }
  }
  ItemConfig2.resolveContent = resolveContent;
  function resolveId(id) {
    if (id === void 0) {
      return ResolvedItemConfig.defaults.id;
    } else {
      if (Array.isArray(id)) {
        if (id.length === 0) {
          return ResolvedItemConfig.defaults.id;
        } else {
          return id[0];
        }
      } else {
        return id;
      }
    }
  }
  ItemConfig2.resolveId = resolveId;
  function resolveSize(size, width, height, rowAndColumnChildLegacySizeDefault) {
    if (size !== void 0) {
      return parseSize(size, [SizeUnitEnum.Percent, SizeUnitEnum.Fractional]);
    } else {
      if (width !== void 0 || height !== void 0) {
        if (width !== void 0) {
          return { size: width, sizeUnit: SizeUnitEnum.Percent };
        } else {
          if (height !== void 0) {
            return { size: height, sizeUnit: SizeUnitEnum.Percent };
          } else {
            throw new UnexpectedUndefinedError("CRS33390");
          }
        }
      } else {
        if (rowAndColumnChildLegacySizeDefault) {
          return { size: 50, sizeUnit: SizeUnitEnum.Percent };
        } else {
          return { size: ResolvedItemConfig.defaults.size, sizeUnit: ResolvedItemConfig.defaults.sizeUnit };
        }
      }
    }
  }
  ItemConfig2.resolveSize = resolveSize;
  function resolveMinSize(minSize, minWidth, minHeight) {
    if (minSize !== void 0) {
      return parseSize(minSize, [SizeUnitEnum.Pixel]);
    } else {
      const minWidthDefined = minWidth !== void 0;
      const minHeightDefined = minHeight !== void 0;
      if (minWidthDefined || minHeightDefined) {
        if (minWidthDefined) {
          return { size: minWidth, sizeUnit: SizeUnitEnum.Pixel };
        } else {
          return { size: minHeight, sizeUnit: SizeUnitEnum.Pixel };
        }
      } else {
        return { size: ResolvedItemConfig.defaults.minSize, sizeUnit: ResolvedItemConfig.defaults.minSizeUnit };
      }
    }
  }
  ItemConfig2.resolveMinSize = resolveMinSize;
  function calculateSizeWidthHeightSpecificationType(config) {
    if (config.size !== void 0) {
      return 1;
    } else {
      if (config.width !== void 0 || config.height !== void 0) {
        return 2;
      } else {
        return 0;
      }
    }
  }
  ItemConfig2.calculateSizeWidthHeightSpecificationType = calculateSizeWidthHeightSpecificationType;
  function isGround(config) {
    return config.type === ItemType.ground;
  }
  ItemConfig2.isGround = isGround;
  function isRow(config) {
    return config.type === ItemType.row;
  }
  ItemConfig2.isRow = isRow;
  function isColumn(config) {
    return config.type === ItemType.column;
  }
  ItemConfig2.isColumn = isColumn;
  function isStack(config) {
    return config.type === ItemType.stack;
  }
  ItemConfig2.isStack = isStack;
  function isComponent(config) {
    return config.type === ItemType.component;
  }
  ItemConfig2.isComponent = isComponent;
})(ItemConfig || (ItemConfig = {}));
var HeaderedItemConfig;
(function(HeaderedItemConfig2) {
  const legacyMaximisedId = "__glMaximised";
  let Header2;
  (function(Header3) {
    function resolve(header, hasHeaders) {
      var _a;
      if (header === void 0 && hasHeaders === void 0) {
        return void 0;
      } else {
        const result = {
          show: (_a = header === null || header === void 0 ? void 0 : header.show) !== null && _a !== void 0 ? _a : hasHeaders === void 0 ? void 0 : hasHeaders ? ResolvedLayoutConfig.Header.defaults.show : false,
          popout: header === null || header === void 0 ? void 0 : header.popout,
          maximise: header === null || header === void 0 ? void 0 : header.maximise,
          close: header === null || header === void 0 ? void 0 : header.close,
          minimise: header === null || header === void 0 ? void 0 : header.minimise,
          tabDropdown: header === null || header === void 0 ? void 0 : header.tabDropdown
        };
        return result;
      }
    }
    Header3.resolve = resolve;
  })(Header2 = HeaderedItemConfig2.Header || (HeaderedItemConfig2.Header = {}));
  function resolveIdAndMaximised(config) {
    let id;
    let legacyId = config.id;
    let legacyMaximised = false;
    if (legacyId === void 0) {
      id = ResolvedItemConfig.defaults.id;
    } else {
      if (Array.isArray(legacyId)) {
        const idx = legacyId.findIndex((id2) => id2 === legacyMaximisedId);
        if (idx > 0) {
          legacyMaximised = true;
          legacyId = legacyId.splice(idx, 1);
        }
        if (legacyId.length > 0) {
          id = legacyId[0];
        } else {
          id = ResolvedItemConfig.defaults.id;
        }
      } else {
        id = legacyId;
      }
    }
    let maximised;
    if (config.maximised !== void 0) {
      maximised = config.maximised;
    } else {
      maximised = legacyMaximised;
    }
    return { id, maximised };
  }
  HeaderedItemConfig2.resolveIdAndMaximised = resolveIdAndMaximised;
})(HeaderedItemConfig || (HeaderedItemConfig = {}));
var StackItemConfig;
(function(StackItemConfig2) {
  function resolve(itemConfig, rowAndColumnChildLegacySizeDefault) {
    var _a, _b;
    const { id, maximised } = HeaderedItemConfig.resolveIdAndMaximised(itemConfig);
    const { size, sizeUnit } = ItemConfig.resolveSize(itemConfig.size, itemConfig.width, itemConfig.height, rowAndColumnChildLegacySizeDefault);
    const { size: minSize, sizeUnit: minSizeUnit } = ItemConfig.resolveMinSize(itemConfig.minSize, itemConfig.minWidth, itemConfig.minHeight);
    const result = {
      type: ItemType.stack,
      content: resolveContent(itemConfig.content),
      size,
      sizeUnit,
      minSize,
      minSizeUnit,
      id,
      maximised,
      isClosable: (_a = itemConfig.isClosable) !== null && _a !== void 0 ? _a : ResolvedItemConfig.defaults.isClosable,
      activeItemIndex: (_b = itemConfig.activeItemIndex) !== null && _b !== void 0 ? _b : ResolvedStackItemConfig.defaultActiveItemIndex,
      header: HeaderedItemConfig.Header.resolve(itemConfig.header, itemConfig.hasHeaders)
    };
    return result;
  }
  StackItemConfig2.resolve = resolve;
  function fromResolved(resolvedConfig) {
    const result = {
      type: ItemType.stack,
      content: fromResolvedContent(resolvedConfig.content),
      size: formatSize(resolvedConfig.size, resolvedConfig.sizeUnit),
      minSize: formatUndefinableSize(resolvedConfig.minSize, resolvedConfig.minSizeUnit),
      id: resolvedConfig.id,
      maximised: resolvedConfig.maximised,
      isClosable: resolvedConfig.isClosable,
      activeItemIndex: resolvedConfig.activeItemIndex,
      header: ResolvedHeaderedItemConfig.Header.createCopy(resolvedConfig.header)
    };
    return result;
  }
  StackItemConfig2.fromResolved = fromResolved;
  function resolveContent(content) {
    if (content === void 0) {
      return [];
    } else {
      const count = content.length;
      const result = new Array(count);
      for (let i = 0; i < count; i++) {
        const childItemConfig = content[i];
        const itemConfig = ItemConfig.resolve(childItemConfig, false);
        if (!ResolvedItemConfig.isComponentItem(itemConfig)) {
          throw new AssertError("UCUSICRC91114", JSON.stringify(itemConfig));
        } else {
          result[i] = itemConfig;
        }
      }
      return result;
    }
  }
  function fromResolvedContent(resolvedContent) {
    const count = resolvedContent.length;
    const result = new Array(count);
    for (let i = 0; i < count; i++) {
      const resolvedContentConfig = resolvedContent[i];
      result[i] = ComponentItemConfig.fromResolved(resolvedContentConfig);
    }
    return result;
  }
})(StackItemConfig || (StackItemConfig = {}));
var ComponentItemConfig;
(function(ComponentItemConfig2) {
  function resolve(itemConfig, rowAndColumnChildLegacySizeDefault) {
    var _a, _b, _c;
    let componentType = itemConfig.componentType;
    if (componentType === void 0) {
      componentType = itemConfig.componentName;
    }
    if (componentType === void 0) {
      throw new Error("ComponentItemConfig.componentType is undefined");
    } else {
      const { id, maximised } = HeaderedItemConfig.resolveIdAndMaximised(itemConfig);
      let title;
      if (itemConfig.title === void 0 || itemConfig.title === "") {
        title = ComponentItemConfig2.componentTypeToTitle(componentType);
      } else {
        title = itemConfig.title;
      }
      const { size, sizeUnit } = ItemConfig.resolveSize(itemConfig.size, itemConfig.width, itemConfig.height, rowAndColumnChildLegacySizeDefault);
      const { size: minSize, sizeUnit: minSizeUnit } = ItemConfig.resolveMinSize(itemConfig.minSize, itemConfig.minWidth, itemConfig.minHeight);
      const result = {
        type: itemConfig.type,
        content: [],
        size,
        sizeUnit,
        minSize,
        minSizeUnit,
        id,
        maximised,
        isClosable: (_a = itemConfig.isClosable) !== null && _a !== void 0 ? _a : ResolvedItemConfig.defaults.isClosable,
        reorderEnabled: (_b = itemConfig.reorderEnabled) !== null && _b !== void 0 ? _b : ResolvedComponentItemConfig.defaultReorderEnabled,
        title,
        header: HeaderedItemConfig.Header.resolve(itemConfig.header, itemConfig.hasHeaders),
        componentType,
        componentState: (_c = itemConfig.componentState) !== null && _c !== void 0 ? _c : {}
      };
      return result;
    }
  }
  ComponentItemConfig2.resolve = resolve;
  function fromResolved(resolvedConfig) {
    const result = {
      type: ItemType.component,
      size: formatSize(resolvedConfig.size, resolvedConfig.sizeUnit),
      minSize: formatUndefinableSize(resolvedConfig.minSize, resolvedConfig.minSizeUnit),
      id: resolvedConfig.id,
      maximised: resolvedConfig.maximised,
      isClosable: resolvedConfig.isClosable,
      reorderEnabled: resolvedConfig.reorderEnabled,
      title: resolvedConfig.title,
      header: ResolvedHeaderedItemConfig.Header.createCopy(resolvedConfig.header),
      componentType: resolvedConfig.componentType,
      componentState: deepExtendValue(void 0, resolvedConfig.componentState)
    };
    return result;
  }
  ComponentItemConfig2.fromResolved = fromResolved;
  function componentTypeToTitle(componentType) {
    const componentTypeType = typeof componentType;
    switch (componentTypeType) {
      case "string":
        return componentType;
      case "number":
        return componentType.toString();
      case "boolean":
        return componentType.toString();
      default:
        return "";
    }
  }
  ComponentItemConfig2.componentTypeToTitle = componentTypeToTitle;
})(ComponentItemConfig || (ComponentItemConfig = {}));
var RowOrColumnItemConfig;
(function(RowOrColumnItemConfig2) {
  function isChildItemConfig(itemConfig) {
    switch (itemConfig.type) {
      case ItemType.row:
      case ItemType.column:
      case ItemType.stack:
      case ItemType.component:
        return true;
      case ItemType.ground:
        return false;
      default:
        throw new UnreachableCaseError("UROCOSPCICIC13687", itemConfig.type);
    }
  }
  RowOrColumnItemConfig2.isChildItemConfig = isChildItemConfig;
  function resolve(itemConfig, rowAndColumnChildLegacySizeDefault) {
    var _a;
    const { size, sizeUnit } = ItemConfig.resolveSize(itemConfig.size, itemConfig.width, itemConfig.height, rowAndColumnChildLegacySizeDefault);
    const { size: minSize, sizeUnit: minSizeUnit } = ItemConfig.resolveMinSize(itemConfig.minSize, itemConfig.minWidth, itemConfig.minHeight);
    const result = {
      type: itemConfig.type,
      content: RowOrColumnItemConfig2.resolveContent(itemConfig.content),
      size,
      sizeUnit,
      minSize,
      minSizeUnit,
      id: ItemConfig.resolveId(itemConfig.id),
      isClosable: (_a = itemConfig.isClosable) !== null && _a !== void 0 ? _a : ResolvedItemConfig.defaults.isClosable
    };
    return result;
  }
  RowOrColumnItemConfig2.resolve = resolve;
  function fromResolved(resolvedConfig) {
    const result = {
      type: resolvedConfig.type,
      content: fromResolvedContent(resolvedConfig.content),
      size: formatSize(resolvedConfig.size, resolvedConfig.sizeUnit),
      minSize: formatUndefinableSize(resolvedConfig.minSize, resolvedConfig.minSizeUnit),
      id: resolvedConfig.id,
      isClosable: resolvedConfig.isClosable
    };
    return result;
  }
  RowOrColumnItemConfig2.fromResolved = fromResolved;
  function resolveContent(content) {
    if (content === void 0) {
      return [];
    } else {
      const count = content.length;
      const childItemConfigs = new Array(count);
      let widthOrHeightSpecifiedAtLeastOnce = false;
      let sizeSpecifiedAtLeastOnce = false;
      for (let i = 0; i < count; i++) {
        const childItemConfig = content[i];
        if (!RowOrColumnItemConfig2.isChildItemConfig(childItemConfig)) {
          throw new ConfigurationError("ItemConfig is not Row, Column or Stack", childItemConfig);
        } else {
          if (!sizeSpecifiedAtLeastOnce) {
            const sizeWidthHeightSpecificationType = ItemConfig.calculateSizeWidthHeightSpecificationType(childItemConfig);
            switch (sizeWidthHeightSpecificationType) {
              case 0:
                break;
              case 2:
                widthOrHeightSpecifiedAtLeastOnce = true;
                break;
              case 1:
                sizeSpecifiedAtLeastOnce = true;
                break;
              default:
                throw new UnreachableCaseError("ROCICRC87556", sizeWidthHeightSpecificationType);
            }
          }
          childItemConfigs[i] = childItemConfig;
        }
      }
      let legacySizeDefault;
      if (sizeSpecifiedAtLeastOnce) {
        legacySizeDefault = false;
      } else {
        if (widthOrHeightSpecifiedAtLeastOnce) {
          legacySizeDefault = true;
        } else {
          legacySizeDefault = false;
        }
      }
      const result = new Array(count);
      for (let i = 0; i < count; i++) {
        const childItemConfig = childItemConfigs[i];
        const resolvedChildItemConfig = ItemConfig.resolve(childItemConfig, legacySizeDefault);
        if (!ResolvedRowOrColumnItemConfig.isChildItemConfig(resolvedChildItemConfig)) {
          throw new AssertError("UROCOSPIC99512", JSON.stringify(resolvedChildItemConfig));
        } else {
          result[i] = resolvedChildItemConfig;
        }
      }
      return result;
    }
  }
  RowOrColumnItemConfig2.resolveContent = resolveContent;
  function fromResolvedContent(resolvedContent) {
    const count = resolvedContent.length;
    const result = new Array(count);
    for (let i = 0; i < count; i++) {
      const resolvedContentConfig = resolvedContent[i];
      const type = resolvedContentConfig.type;
      let contentConfig;
      switch (type) {
        case ItemType.row:
        case ItemType.column:
          contentConfig = RowOrColumnItemConfig2.fromResolved(resolvedContentConfig);
          break;
        case ItemType.stack:
          contentConfig = StackItemConfig.fromResolved(resolvedContentConfig);
          break;
        case ItemType.component:
          contentConfig = ComponentItemConfig.fromResolved(resolvedContentConfig);
          break;
        default:
          throw new UnreachableCaseError("ROCICFRC44797", type);
      }
      result[i] = contentConfig;
    }
    return result;
  }
})(RowOrColumnItemConfig || (RowOrColumnItemConfig = {}));
var RootItemConfig;
(function(RootItemConfig2) {
  function isRootItemConfig(itemConfig) {
    switch (itemConfig.type) {
      case ItemType.row:
      case ItemType.column:
      case ItemType.stack:
      case ItemType.component:
        return true;
      case ItemType.ground:
        return false;
      default:
        throw new UnreachableCaseError("URICIR23687", itemConfig.type);
    }
  }
  RootItemConfig2.isRootItemConfig = isRootItemConfig;
  function resolve(itemConfig) {
    if (itemConfig === void 0) {
      return void 0;
    } else {
      const result = ItemConfig.resolve(itemConfig, false);
      if (!ResolvedRootItemConfig.isRootItemConfig(result)) {
        throw new ConfigurationError("ItemConfig is not Row, Column or Stack", JSON.stringify(itemConfig));
      } else {
        return result;
      }
    }
  }
  RootItemConfig2.resolve = resolve;
  function fromResolvedOrUndefined(resolvedItemConfig) {
    if (resolvedItemConfig === void 0) {
      return void 0;
    } else {
      const type = resolvedItemConfig.type;
      switch (type) {
        case ItemType.row:
        case ItemType.column:
          return RowOrColumnItemConfig.fromResolved(resolvedItemConfig);
        case ItemType.stack:
          return StackItemConfig.fromResolved(resolvedItemConfig);
        case ItemType.component:
          return ComponentItemConfig.fromResolved(resolvedItemConfig);
        default:
          throw new UnreachableCaseError("RICFROU89921", type);
      }
    }
  }
  RootItemConfig2.fromResolvedOrUndefined = fromResolvedOrUndefined;
})(RootItemConfig || (RootItemConfig = {}));
var LayoutConfig;
(function(LayoutConfig2) {
  let Settings;
  (function(Settings2) {
    function resolve2(settings) {
      var _a, _b, _c, _d, _e, _f, _g, _h, _j, _k;
      const result = {
        constrainDragToContainer: (_a = settings === null || settings === void 0 ? void 0 : settings.constrainDragToContainer) !== null && _a !== void 0 ? _a : ResolvedLayoutConfig.Settings.defaults.constrainDragToContainer,
        reorderEnabled: (_b = settings === null || settings === void 0 ? void 0 : settings.reorderEnabled) !== null && _b !== void 0 ? _b : ResolvedLayoutConfig.Settings.defaults.reorderEnabled,
        popoutWholeStack: (_c = settings === null || settings === void 0 ? void 0 : settings.popoutWholeStack) !== null && _c !== void 0 ? _c : ResolvedLayoutConfig.Settings.defaults.popoutWholeStack,
        blockedPopoutsThrowError: (_d = settings === null || settings === void 0 ? void 0 : settings.blockedPopoutsThrowError) !== null && _d !== void 0 ? _d : ResolvedLayoutConfig.Settings.defaults.blockedPopoutsThrowError,
        closePopoutsOnUnload: (_e = settings === null || settings === void 0 ? void 0 : settings.closePopoutsOnUnload) !== null && _e !== void 0 ? _e : ResolvedLayoutConfig.Settings.defaults.closePopoutsOnUnload,
        responsiveMode: (_f = settings === null || settings === void 0 ? void 0 : settings.responsiveMode) !== null && _f !== void 0 ? _f : ResolvedLayoutConfig.Settings.defaults.responsiveMode,
        tabOverlapAllowance: (_g = settings === null || settings === void 0 ? void 0 : settings.tabOverlapAllowance) !== null && _g !== void 0 ? _g : ResolvedLayoutConfig.Settings.defaults.tabOverlapAllowance,
        reorderOnTabMenuClick: (_h = settings === null || settings === void 0 ? void 0 : settings.reorderOnTabMenuClick) !== null && _h !== void 0 ? _h : ResolvedLayoutConfig.Settings.defaults.reorderOnTabMenuClick,
        tabControlOffset: (_j = settings === null || settings === void 0 ? void 0 : settings.tabControlOffset) !== null && _j !== void 0 ? _j : ResolvedLayoutConfig.Settings.defaults.tabControlOffset,
        popInOnClose: (_k = settings === null || settings === void 0 ? void 0 : settings.popInOnClose) !== null && _k !== void 0 ? _k : ResolvedLayoutConfig.Settings.defaults.popInOnClose
      };
      return result;
    }
    Settings2.resolve = resolve2;
  })(Settings = LayoutConfig2.Settings || (LayoutConfig2.Settings = {}));
  let Dimensions;
  (function(Dimensions2) {
    function resolve2(dimensions) {
      var _a, _b, _c, _d, _e;
      const { size: defaultMinItemHeight, sizeUnit: defaultMinItemHeightUnit } = Dimensions2.resolveDefaultMinItemHeight(dimensions);
      const { size: defaultMinItemWidth, sizeUnit: defaultMinItemWidthUnit } = Dimensions2.resolveDefaultMinItemWidth(dimensions);
      const result = {
        borderWidth: (_a = dimensions === null || dimensions === void 0 ? void 0 : dimensions.borderWidth) !== null && _a !== void 0 ? _a : ResolvedLayoutConfig.Dimensions.defaults.borderWidth,
        borderGrabWidth: (_b = dimensions === null || dimensions === void 0 ? void 0 : dimensions.borderGrabWidth) !== null && _b !== void 0 ? _b : ResolvedLayoutConfig.Dimensions.defaults.borderGrabWidth,
        defaultMinItemHeight,
        defaultMinItemHeightUnit,
        defaultMinItemWidth,
        defaultMinItemWidthUnit,
        headerHeight: (_c = dimensions === null || dimensions === void 0 ? void 0 : dimensions.headerHeight) !== null && _c !== void 0 ? _c : ResolvedLayoutConfig.Dimensions.defaults.headerHeight,
        dragProxyWidth: (_d = dimensions === null || dimensions === void 0 ? void 0 : dimensions.dragProxyWidth) !== null && _d !== void 0 ? _d : ResolvedLayoutConfig.Dimensions.defaults.dragProxyWidth,
        dragProxyHeight: (_e = dimensions === null || dimensions === void 0 ? void 0 : dimensions.dragProxyHeight) !== null && _e !== void 0 ? _e : ResolvedLayoutConfig.Dimensions.defaults.dragProxyHeight
      };
      return result;
    }
    Dimensions2.resolve = resolve2;
    function fromResolved2(resolvedDimensions) {
      const result = {
        borderWidth: resolvedDimensions.borderWidth,
        borderGrabWidth: resolvedDimensions.borderGrabWidth,
        defaultMinItemHeight: formatSize(resolvedDimensions.defaultMinItemHeight, resolvedDimensions.defaultMinItemHeightUnit),
        defaultMinItemWidth: formatSize(resolvedDimensions.defaultMinItemWidth, resolvedDimensions.defaultMinItemWidthUnit),
        headerHeight: resolvedDimensions.headerHeight,
        dragProxyWidth: resolvedDimensions.dragProxyWidth,
        dragProxyHeight: resolvedDimensions.dragProxyHeight
      };
      return result;
    }
    Dimensions2.fromResolved = fromResolved2;
    function resolveDefaultMinItemHeight(dimensions) {
      const height = dimensions === null || dimensions === void 0 ? void 0 : dimensions.defaultMinItemHeight;
      if (height === void 0) {
        return { size: ResolvedLayoutConfig.Dimensions.defaults.defaultMinItemHeight, sizeUnit: ResolvedLayoutConfig.Dimensions.defaults.defaultMinItemHeightUnit };
      } else {
        return parseSize(height, [SizeUnitEnum.Pixel]);
      }
    }
    Dimensions2.resolveDefaultMinItemHeight = resolveDefaultMinItemHeight;
    function resolveDefaultMinItemWidth(dimensions) {
      const width = dimensions === null || dimensions === void 0 ? void 0 : dimensions.defaultMinItemWidth;
      if (width === void 0) {
        return { size: ResolvedLayoutConfig.Dimensions.defaults.defaultMinItemWidth, sizeUnit: ResolvedLayoutConfig.Dimensions.defaults.defaultMinItemWidthUnit };
      } else {
        return parseSize(width, [SizeUnitEnum.Pixel]);
      }
    }
    Dimensions2.resolveDefaultMinItemWidth = resolveDefaultMinItemWidth;
  })(Dimensions = LayoutConfig2.Dimensions || (LayoutConfig2.Dimensions = {}));
  let Header2;
  (function(Header3) {
    function resolve2(header, settings, labels) {
      var _a, _b, _c, _d, _e, _f, _g, _h, _j, _k, _l, _m;
      let show;
      if ((header === null || header === void 0 ? void 0 : header.show) !== void 0) {
        show = header.show;
      } else {
        if (settings !== void 0 && settings.hasHeaders !== void 0) {
          show = settings.hasHeaders ? ResolvedLayoutConfig.Header.defaults.show : false;
        } else {
          show = ResolvedLayoutConfig.Header.defaults.show;
        }
      }
      const result = {
        show,
        popout: (_b = (_a = header === null || header === void 0 ? void 0 : header.popout) !== null && _a !== void 0 ? _a : labels === null || labels === void 0 ? void 0 : labels.popout) !== null && _b !== void 0 ? _b : (settings === null || settings === void 0 ? void 0 : settings.showPopoutIcon) === false ? false : ResolvedLayoutConfig.Header.defaults.popout,
        dock: (_d = (_c = header === null || header === void 0 ? void 0 : header.popin) !== null && _c !== void 0 ? _c : labels === null || labels === void 0 ? void 0 : labels.popin) !== null && _d !== void 0 ? _d : ResolvedLayoutConfig.Header.defaults.dock,
        maximise: (_f = (_e = header === null || header === void 0 ? void 0 : header.maximise) !== null && _e !== void 0 ? _e : labels === null || labels === void 0 ? void 0 : labels.maximise) !== null && _f !== void 0 ? _f : (settings === null || settings === void 0 ? void 0 : settings.showMaximiseIcon) === false ? false : ResolvedLayoutConfig.Header.defaults.maximise,
        close: (_h = (_g = header === null || header === void 0 ? void 0 : header.close) !== null && _g !== void 0 ? _g : labels === null || labels === void 0 ? void 0 : labels.close) !== null && _h !== void 0 ? _h : (settings === null || settings === void 0 ? void 0 : settings.showCloseIcon) === false ? false : ResolvedLayoutConfig.Header.defaults.close,
        minimise: (_k = (_j = header === null || header === void 0 ? void 0 : header.minimise) !== null && _j !== void 0 ? _j : labels === null || labels === void 0 ? void 0 : labels.minimise) !== null && _k !== void 0 ? _k : ResolvedLayoutConfig.Header.defaults.minimise,
        tabDropdown: (_m = (_l = header === null || header === void 0 ? void 0 : header.tabDropdown) !== null && _l !== void 0 ? _l : labels === null || labels === void 0 ? void 0 : labels.tabDropdown) !== null && _m !== void 0 ? _m : ResolvedLayoutConfig.Header.defaults.tabDropdown
      };
      return result;
    }
    Header3.resolve = resolve2;
  })(Header2 = LayoutConfig2.Header || (LayoutConfig2.Header = {}));
  function isPopout(config) {
    return "parentId" in config || "indexInParent" in config || "window" in config;
  }
  LayoutConfig2.isPopout = isPopout;
  function resolve(layoutConfig) {
    if (isPopout(layoutConfig)) {
      return PopoutLayoutConfig.resolve(layoutConfig);
    } else {
      let root;
      if (layoutConfig.root !== void 0) {
        root = layoutConfig.root;
      } else {
        if (layoutConfig.content !== void 0 && layoutConfig.content.length > 0) {
          root = layoutConfig.content[0];
        } else {
          root = void 0;
        }
      }
      const config = {
        resolved: true,
        root: RootItemConfig.resolve(root),
        openPopouts: LayoutConfig2.resolveOpenPopouts(layoutConfig.openPopouts),
        dimensions: LayoutConfig2.Dimensions.resolve(layoutConfig.dimensions),
        settings: LayoutConfig2.Settings.resolve(layoutConfig.settings),
        header: LayoutConfig2.Header.resolve(layoutConfig.header, layoutConfig.settings, layoutConfig.labels)
      };
      return config;
    }
  }
  LayoutConfig2.resolve = resolve;
  function fromResolved(config) {
    const result = {
      root: RootItemConfig.fromResolvedOrUndefined(config.root),
      openPopouts: PopoutLayoutConfig.fromResolvedArray(config.openPopouts),
      settings: ResolvedLayoutConfig.Settings.createCopy(config.settings),
      dimensions: LayoutConfig2.Dimensions.fromResolved(config.dimensions),
      header: ResolvedLayoutConfig.Header.createCopy(config.header)
    };
    return result;
  }
  LayoutConfig2.fromResolved = fromResolved;
  function isResolved(configOrResolvedConfig) {
    const config = configOrResolvedConfig;
    return config.resolved !== void 0 && config.resolved === true;
  }
  LayoutConfig2.isResolved = isResolved;
  function resolveOpenPopouts(popoutConfigs) {
    if (popoutConfigs === void 0) {
      return [];
    } else {
      const count = popoutConfigs.length;
      const result = new Array(count);
      for (let i = 0; i < count; i++) {
        result[i] = PopoutLayoutConfig.resolve(popoutConfigs[i]);
      }
      return result;
    }
  }
  LayoutConfig2.resolveOpenPopouts = resolveOpenPopouts;
})(LayoutConfig || (LayoutConfig = {}));
var PopoutLayoutConfig;
(function(PopoutLayoutConfig2) {
  let Window;
  (function(Window2) {
    function resolve2(window2, dimensions) {
      var _a, _b, _c, _d, _e, _f, _g, _h;
      let result;
      const defaults = ResolvedPopoutLayoutConfig.Window.defaults;
      if (window2 !== void 0) {
        result = {
          width: (_a = window2.width) !== null && _a !== void 0 ? _a : defaults.width,
          height: (_b = window2.height) !== null && _b !== void 0 ? _b : defaults.height,
          left: (_c = window2.left) !== null && _c !== void 0 ? _c : defaults.left,
          top: (_d = window2.top) !== null && _d !== void 0 ? _d : defaults.top
        };
      } else {
        result = {
          width: (_e = dimensions === null || dimensions === void 0 ? void 0 : dimensions.width) !== null && _e !== void 0 ? _e : defaults.width,
          height: (_f = dimensions === null || dimensions === void 0 ? void 0 : dimensions.height) !== null && _f !== void 0 ? _f : defaults.height,
          left: (_g = dimensions === null || dimensions === void 0 ? void 0 : dimensions.left) !== null && _g !== void 0 ? _g : defaults.left,
          top: (_h = dimensions === null || dimensions === void 0 ? void 0 : dimensions.top) !== null && _h !== void 0 ? _h : defaults.top
        };
      }
      return result;
    }
    Window2.resolve = resolve2;
    function fromResolved2(resolvedWindow) {
      const result = {
        width: resolvedWindow.width === null ? void 0 : resolvedWindow.width,
        height: resolvedWindow.height === null ? void 0 : resolvedWindow.height,
        left: resolvedWindow.left === null ? void 0 : resolvedWindow.left,
        top: resolvedWindow.top === null ? void 0 : resolvedWindow.top
      };
      return result;
    }
    Window2.fromResolved = fromResolved2;
  })(Window = PopoutLayoutConfig2.Window || (PopoutLayoutConfig2.Window = {}));
  function resolve(popoutConfig) {
    var _a, _b;
    let root;
    if (popoutConfig.root !== void 0) {
      root = popoutConfig.root;
    } else {
      if (popoutConfig.content !== void 0 && popoutConfig.content.length > 0) {
        root = popoutConfig.content[0];
      } else {
        root = void 0;
      }
    }
    const config = {
      root: RootItemConfig.resolve(root),
      openPopouts: LayoutConfig.resolveOpenPopouts(popoutConfig.openPopouts),
      dimensions: LayoutConfig.Dimensions.resolve(popoutConfig.dimensions),
      settings: LayoutConfig.Settings.resolve(popoutConfig.settings),
      header: LayoutConfig.Header.resolve(popoutConfig.header, popoutConfig.settings, popoutConfig.labels),
      parentId: (_a = popoutConfig.parentId) !== null && _a !== void 0 ? _a : null,
      indexInParent: (_b = popoutConfig.indexInParent) !== null && _b !== void 0 ? _b : null,
      window: PopoutLayoutConfig2.Window.resolve(popoutConfig.window, popoutConfig.dimensions),
      resolved: true
    };
    return config;
  }
  PopoutLayoutConfig2.resolve = resolve;
  function fromResolved(resolvedConfig) {
    const result = {
      root: RootItemConfig.fromResolvedOrUndefined(resolvedConfig.root),
      openPopouts: fromResolvedArray(resolvedConfig.openPopouts),
      dimensions: LayoutConfig.Dimensions.fromResolved(resolvedConfig.dimensions),
      settings: ResolvedLayoutConfig.Settings.createCopy(resolvedConfig.settings),
      header: ResolvedLayoutConfig.Header.createCopy(resolvedConfig.header),
      parentId: resolvedConfig.parentId,
      indexInParent: resolvedConfig.indexInParent,
      window: PopoutLayoutConfig2.Window.fromResolved(resolvedConfig.window)
    };
    return result;
  }
  PopoutLayoutConfig2.fromResolved = fromResolved;
  function fromResolvedArray(resolvedArray) {
    const resolvedOpenPopoutCount = resolvedArray.length;
    const result = new Array(resolvedOpenPopoutCount);
    for (let i = 0; i < resolvedOpenPopoutCount; i++) {
      const resolvedOpenPopout = resolvedArray[i];
      result[i] = PopoutLayoutConfig2.fromResolved(resolvedOpenPopout);
    }
    return result;
  }
  PopoutLayoutConfig2.fromResolvedArray = fromResolvedArray;
})(PopoutLayoutConfig || (PopoutLayoutConfig = {}));
function parseSize(sizeString, allowableSizeUnits) {
  const { numericPart: digitsPart, firstNonNumericCharPart: firstNonDigitPart } = splitStringAtFirstNonNumericChar(sizeString);
  const size = Number.parseInt(digitsPart, 10);
  if (isNaN(size)) {
    throw new ConfigurationError(`${i18nStrings[
      7
      /* InvalidNumberPartInSizeString */
    ]}: ${sizeString}`);
  } else {
    const sizeUnit = SizeUnitEnum.tryParse(firstNonDigitPart);
    if (sizeUnit === void 0) {
      throw new ConfigurationError(`${i18nStrings[
        8
        /* UnknownUnitInSizeString */
      ]}: ${sizeString}`);
    } else {
      if (!allowableSizeUnits.includes(sizeUnit)) {
        throw new ConfigurationError(`${i18nStrings[
          9
          /* UnsupportedUnitInSizeString */
        ]}: ${sizeString}`);
      } else {
        return { size, sizeUnit };
      }
    }
  }
}
function formatSize(size, sizeUnit) {
  return size.toString(10) + SizeUnitEnum.format(sizeUnit);
}
function formatUndefinableSize(size, sizeUnit) {
  if (size === void 0) {
    return void 0;
  } else {
    return size.toString(10) + SizeUnitEnum.format(sizeUnit);
  }
}

// dist/esm/ts/utils/event-emitter.js
var EventEmitter = class _EventEmitter {
  constructor() {
    this._allEventSubscriptions = [];
    this._subscriptionsMap = /* @__PURE__ */ new Map();
    this.unbind = this.removeEventListener;
    this.trigger = this.emit;
  }
  // eslint-disable-next-line @typescript-eslint/no-unused-vars
  tryBubbleEvent(name, args) {
  }
  /**
   * Emit an event and notify listeners
   *
   * @param eventName - The name of the event
   * @param args - Additional arguments that will be passed to the listener
   */
  emit(eventName, ...args) {
    let subcriptions = this._subscriptionsMap.get(eventName);
    if (subcriptions !== void 0) {
      subcriptions = subcriptions.slice();
      for (let i = 0; i < subcriptions.length; i++) {
        const subscription = subcriptions[i];
        subscription(...args);
      }
    }
    this.emitAllEvent(eventName, args);
    this.tryBubbleEvent(eventName, args);
  }
  /** @internal */
  emitUnknown(eventName, ...args) {
    let subs = this._subscriptionsMap.get(eventName);
    if (subs !== void 0) {
      subs = subs.slice();
      for (let i = 0; i < subs.length; i++) {
        subs[i](...args);
      }
    }
    this.emitAllEvent(eventName, args);
    this.tryBubbleEvent(eventName, args);
  }
  /* @internal **/
  emitBaseBubblingEvent(eventName) {
    const event = new _EventEmitter.BubblingEvent(eventName, this);
    this.emitUnknown(eventName, event);
  }
  /** @internal */
  emitUnknownBubblingEvent(eventName) {
    const event = new _EventEmitter.BubblingEvent(eventName, this);
    this.emitUnknown(eventName, event);
  }
  /**
   * Removes a listener for an event.
   * @param eventName - The name of the event
   * @param callback - The previously registered callback method (optional)
   */
  removeEventListener(eventName, callback) {
    const unknownCallback = callback;
    this.removeUnknownEventListener(eventName, unknownCallback);
  }
  off(eventName, callback) {
    this.removeEventListener(eventName, callback);
  }
  /**
   * Listen for events
   *
   * @param eventName - The name of the event to listen to
   * @param callback - The callback to execute when the event occurs
   */
  addEventListener(eventName, callback) {
    const unknownCallback = callback;
    this.addUnknownEventListener(eventName, unknownCallback);
  }
  on(eventName, callback) {
    this.addEventListener(eventName, callback);
  }
  /** @internal */
  addUnknownEventListener(eventName, callback) {
    if (eventName === _EventEmitter.ALL_EVENT) {
      this._allEventSubscriptions.push(callback);
    } else {
      let subscriptions = this._subscriptionsMap.get(eventName);
      if (subscriptions !== void 0) {
        subscriptions.push(callback);
      } else {
        subscriptions = [callback];
        this._subscriptionsMap.set(eventName, subscriptions);
      }
    }
  }
  /** @internal */
  removeUnknownEventListener(eventName, callback) {
    if (eventName === _EventEmitter.ALL_EVENT) {
      this.removeSubscription(eventName, this._allEventSubscriptions, callback);
    } else {
      const subscriptions = this._subscriptionsMap.get(eventName);
      if (subscriptions === void 0) {
        throw new Error("No subscribtions to unsubscribe for event " + eventName);
      } else {
        this.removeSubscription(eventName, subscriptions, callback);
      }
    }
  }
  /** @internal */
  removeSubscription(eventName, subscriptions, callback) {
    const idx = subscriptions.indexOf(callback);
    if (idx < 0) {
      throw new Error("Nothing to unbind for " + eventName);
    } else {
      subscriptions.splice(idx, 1);
    }
  }
  /** @internal */
  emitAllEvent(eventName, args) {
    const allEventSubscriptionsCount = this._allEventSubscriptions.length;
    if (allEventSubscriptionsCount > 0) {
      const unknownArgs = args.slice();
      unknownArgs.unshift(eventName);
      const allEventSubcriptions = this._allEventSubscriptions.slice();
      for (let i = 0; i < allEventSubscriptionsCount; i++) {
        allEventSubcriptions[i](...unknownArgs);
      }
    }
  }
};
(function(EventEmitter2) {
  EventEmitter2.ALL_EVENT = "__all";
  EventEmitter2.headerClickEventName = "stackHeaderClick";
  EventEmitter2.headerTouchStartEventName = "stackHeaderTouchStart";
  class BubblingEvent {
    /** @internal */
    constructor(_name, _target) {
      this._name = _name;
      this._target = _target;
      this._isPropagationStopped = false;
    }
    get name() {
      return this._name;
    }
    get target() {
      return this._target;
    }
    /** @deprecated Use {@link (EventEmitter:namespace).(BubblingEvent:class).target} instead */
    get origin() {
      return this._target;
    }
    get isPropagationStopped() {
      return this._isPropagationStopped;
    }
    stopPropagation() {
      this._isPropagationStopped = true;
    }
  }
  EventEmitter2.BubblingEvent = BubblingEvent;
  class ClickBubblingEvent extends BubblingEvent {
    /** @internal */
    constructor(name, target, _mouseEvent) {
      super(name, target);
      this._mouseEvent = _mouseEvent;
    }
    get mouseEvent() {
      return this._mouseEvent;
    }
  }
  EventEmitter2.ClickBubblingEvent = ClickBubblingEvent;
  class TouchStartBubblingEvent extends BubblingEvent {
    /** @internal */
    constructor(name, target, _touchEvent) {
      super(name, target);
      this._touchEvent = _touchEvent;
    }
    get touchEvent() {
      return this._touchEvent;
    }
  }
  EventEmitter2.TouchStartBubblingEvent = TouchStartBubblingEvent;
})(EventEmitter || (EventEmitter = {}));

// dist/esm/ts/container/component-container.js
var ComponentContainer = class extends EventEmitter {
  /** @internal */
  constructor(_config, _parent, _layoutManager, _element, _updateItemConfigEvent, _showEvent, _hideEvent, _focusEvent, _blurEvent) {
    super();
    this._config = _config;
    this._parent = _parent;
    this._layoutManager = _layoutManager;
    this._element = _element;
    this._updateItemConfigEvent = _updateItemConfigEvent;
    this._showEvent = _showEvent;
    this._hideEvent = _hideEvent;
    this._focusEvent = _focusEvent;
    this._blurEvent = _blurEvent;
    this._stackMaximised = false;
    this._width = 0;
    this._height = 0;
    this._visible = true;
    this._isShownWithZeroDimensions = true;
    this._componentType = _config.componentType;
    this._isClosable = _config.isClosable;
    this._initialState = _config.componentState;
    this._state = this._initialState;
    this._boundComponent = this.layoutManager.bindComponent(this, _config);
    this.updateElementPositionPropertyFromBoundComponent();
  }
  get width() {
    return this._width;
  }
  get height() {
    return this._height;
  }
  get parent() {
    return this._parent;
  }
  /** @internal @deprecated use {@link (ComponentContainer:class).componentType} */
  get componentName() {
    return this._componentType;
  }
  get componentType() {
    return this._componentType;
  }
  get virtual() {
    return this._boundComponent.virtual;
  }
  get component() {
    return this._boundComponent.component;
  }
  get tab() {
    return this._tab;
  }
  get title() {
    return this._parent.title;
  }
  get layoutManager() {
    return this._layoutManager;
  }
  get isHidden() {
    return !this._visible;
  }
  get visible() {
    return this._visible;
  }
  get state() {
    return this._state;
  }
  /** Return the initial component state */
  get initialState() {
    return this._initialState;
  }
  /** The inner DOM element where the container's content is intended to live in */
  get element() {
    return this._element;
  }
  /** @internal */
  destroy() {
    this.releaseComponent();
    this.stateRequestEvent = void 0;
    this.emit("destroy");
  }
  /** @deprecated use {@link (ComponentContainer:class).element } */
  getElement() {
    return this._element;
  }
  /**
   * Hides the container's component item (and hence, the container) if not already hidden.
   * Emits hide event prior to hiding the container.
   */
  hide() {
    this._hideEvent();
  }
  /**
   * Shows the container's component item (and hence, the container) if not visible.
   * Emits show event prior to hiding the container.
   */
  show() {
    this._showEvent();
  }
  /**
   * Focus this component in Layout.
   */
  focus(suppressEvent = false) {
    this._focusEvent(suppressEvent);
  }
  /**
   * Remove focus from this component in Layout.
   */
  blur(suppressEvent = false) {
    this._blurEvent(suppressEvent);
  }
  /**
   * Set the size from within the container. Traverses up
   * the item tree until it finds a row or column element
   * and resizes its items accordingly.
   *
   * If this container isn't a descendant of a row or column
   * it returns false
   * @param width - The new width in pixel
   * @param height - The new height in pixel
   *
   * @returns resizeSuccesful
   *
   * @internal
   */
  setSize(width, height) {
    let ancestorItem = this._parent;
    if (ancestorItem.isColumn || ancestorItem.isRow || ancestorItem.parent === null) {
      throw new AssertError("ICSSPRC", "ComponentContainer cannot have RowColumn Parent");
    } else {
      let ancestorChildItem;
      do {
        ancestorChildItem = ancestorItem;
        ancestorItem = ancestorItem.parent;
      } while (ancestorItem !== null && !ancestorItem.isColumn && !ancestorItem.isRow);
      if (ancestorItem === null) {
        return false;
      } else {
        const direction = ancestorItem.isColumn ? "height" : "width";
        const currentSize = this[direction];
        if (currentSize === null) {
          throw new UnexpectedNullError("ICSSCS11194");
        } else {
          const newSize = direction === "height" ? height : width;
          const totalPixel = currentSize * (1 / (ancestorChildItem.size / 100));
          const percentage = newSize / totalPixel * 100;
          const delta = (ancestorChildItem.size - percentage) / (ancestorItem.contentItems.length - 1);
          for (let i = 0; i < ancestorItem.contentItems.length; i++) {
            const ancestorItemContentItem = ancestorItem.contentItems[i];
            if (ancestorItemContentItem === ancestorChildItem) {
              ancestorItemContentItem.size = percentage;
            } else {
              ancestorItemContentItem.size += delta;
            }
          }
          ancestorItem.updateSize(false);
          return true;
        }
      }
    }
  }
  /**
   * Closes the container if it is closable. Can be called by
   * both the component within at as well as the contentItem containing
   * it. Emits a close event before the container itself is closed.
   */
  close() {
    if (this._isClosable) {
      this.emit("close");
      this._parent.close();
    }
  }
  /** Replaces component without affecting layout */
  replaceComponent(itemConfig) {
    this.releaseComponent();
    if (!ItemConfig.isComponent(itemConfig)) {
      throw new Error("ReplaceComponent not passed a component ItemConfig");
    } else {
      const config = ComponentItemConfig.resolve(itemConfig, false);
      this._initialState = config.componentState;
      this._state = this._initialState;
      this._componentType = config.componentType;
      this._updateItemConfigEvent(config);
      this._boundComponent = this.layoutManager.bindComponent(this, config);
      this.updateElementPositionPropertyFromBoundComponent();
      if (this._boundComponent.virtual) {
        if (this.virtualVisibilityChangeRequiredEvent !== void 0) {
          this.virtualVisibilityChangeRequiredEvent(this, this._visible);
        }
        if (this.virtualRectingRequiredEvent !== void 0) {
          this._layoutManager.fireBeforeVirtualRectingEvent(1);
          try {
            this.virtualRectingRequiredEvent(this, this._width, this._height);
          } finally {
            this._layoutManager.fireAfterVirtualRectingEvent();
          }
        }
        this.setBaseLogicalZIndex();
      }
      this.emit("stateChanged");
    }
  }
  /**
   * Returns the initial component state or the latest passed in setState()
   * @returns state
   * @deprecated Use {@link (ComponentContainer:class).initialState}
   */
  getState() {
    return this._state;
  }
  /**
   * Merges the provided state into the current one
   * @deprecated Use {@link (ComponentContainer:class).stateRequestEvent}
   */
  extendState(state) {
    const extendedState = deepExtend(this._state, state);
    this.setState(extendedState);
  }
  /**
   * Sets the component state
   * @deprecated Use {@link (ComponentContainer:class).stateRequestEvent}
   */
  setState(state) {
    this._state = state;
    this._parent.emitBaseBubblingEvent("stateChanged");
  }
  /**
   * Set's the components title
   */
  setTitle(title) {
    this._parent.setTitle(title);
  }
  /** @internal */
  setTab(tab) {
    this._tab = tab;
    this.emit("tab", tab);
  }
  /** @internal */
  setVisibility(value) {
    if (this._boundComponent.virtual) {
      if (this.virtualVisibilityChangeRequiredEvent !== void 0) {
        this.virtualVisibilityChangeRequiredEvent(this, value);
      }
    }
    if (value) {
      if (!this._visible) {
        this._visible = true;
        if (this._height === 0 && this._width === 0) {
          this._isShownWithZeroDimensions = true;
        } else {
          this._isShownWithZeroDimensions = false;
          this.setSizeToNodeSize(this._width, this._height, true);
          this.emitShow();
        }
      } else {
        if (this._isShownWithZeroDimensions && (this._height !== 0 || this._width !== 0)) {
          this._isShownWithZeroDimensions = false;
          this.setSizeToNodeSize(this._width, this._height, true);
          this.emitShow();
        }
      }
    } else {
      if (this._visible) {
        this._visible = false;
        this._isShownWithZeroDimensions = false;
        this.emitHide();
      }
    }
  }
  setBaseLogicalZIndex() {
    this.setLogicalZIndex(LogicalZIndex.base);
  }
  setLogicalZIndex(logicalZIndex) {
    if (logicalZIndex !== this._logicalZIndex) {
      this._logicalZIndex = logicalZIndex;
      this.notifyVirtualZIndexChangeRequired();
    }
  }
  /**
   * Set the container's size, but considered temporary (for dragging)
   * so don't emit any events.
   * @internal
   */
  enterDragMode(width, height) {
    this._width = width;
    this._height = height;
    setElementWidth(this._element, width);
    setElementHeight(this._element, height);
    this.setLogicalZIndex(LogicalZIndex.drag);
    this.drag();
  }
  /** @internal */
  exitDragMode() {
    this.setBaseLogicalZIndex();
  }
  /** @internal */
  enterStackMaximised() {
    this._stackMaximised = true;
    this.setLogicalZIndex(LogicalZIndex.stackMaximised);
  }
  /** @internal */
  exitStackMaximised() {
    this.setBaseLogicalZIndex();
    this._stackMaximised = false;
  }
  /** @internal */
  drag() {
    if (this._boundComponent.virtual) {
      if (this.virtualRectingRequiredEvent !== void 0) {
        this._layoutManager.fireBeforeVirtualRectingEvent(1);
        try {
          this.virtualRectingRequiredEvent(this, this._width, this._height);
        } finally {
          this._layoutManager.fireAfterVirtualRectingEvent();
        }
      }
    }
  }
  /**
   * Sets the container's size. Called by the container's component item.
   * To instead set the size programmatically from within the component itself,
   * use the public setSize method
   * @param width - in px
   * @param height - in px
   * @param force - set even if no change
   * @internal
   */
  setSizeToNodeSize(width, height, force) {
    if (width !== this._width || height !== this._height || force) {
      this._width = width;
      this._height = height;
      setElementWidth(this._element, width);
      setElementHeight(this._element, height);
      if (this._boundComponent.virtual) {
        this.addVirtualSizedContainerToLayoutManager();
      } else {
        this.emit("resize");
        this.checkShownFromZeroDimensions();
      }
    }
  }
  /** @internal */
  notifyVirtualRectingRequired() {
    if (this.virtualRectingRequiredEvent !== void 0) {
      this.virtualRectingRequiredEvent(this, this._width, this._height);
      this.emit("resize");
      this.checkShownFromZeroDimensions();
    }
  }
  /** @internal */
  notifyVirtualZIndexChangeRequired() {
    if (this.virtualZIndexChangeRequiredEvent !== void 0) {
      const logicalZIndex = this._logicalZIndex;
      const defaultZIndex = LogicalZIndexToDefaultMap[logicalZIndex];
      this.virtualZIndexChangeRequiredEvent(this, logicalZIndex, defaultZIndex);
    }
  }
  /** @internal */
  updateElementPositionPropertyFromBoundComponent() {
    if (this._boundComponent.virtual) {
      this._element.style.position = "static";
    } else {
      this._element.style.position = "";
    }
  }
  /** @internal */
  addVirtualSizedContainerToLayoutManager() {
    this._layoutManager.beginVirtualSizedContainerAdding();
    try {
      this._layoutManager.addVirtualSizedContainer(this);
    } finally {
      this._layoutManager.endVirtualSizedContainerAdding();
    }
  }
  /** @internal */
  checkShownFromZeroDimensions() {
    if (this._isShownWithZeroDimensions && (this._height !== 0 || this._width !== 0)) {
      this._isShownWithZeroDimensions = false;
      this.emitShow();
    }
  }
  /** @internal */
  emitShow() {
    this.emit("shown");
    this.emit("show");
  }
  /** @internal */
  emitHide() {
    this.emit("hide");
  }
  /** @internal */
  releaseComponent() {
    if (this._stackMaximised) {
      this.exitStackMaximised();
    }
    this.emit("beforeComponentRelease", this._boundComponent.component);
    this.layoutManager.unbindComponent(this, this._boundComponent.virtual, this._boundComponent.component);
  }
};

// dist/esm/ts/controls/browser-popout.js
var BrowserPopout = class extends EventEmitter {
  /**
   * @param _config - GoldenLayout item config
   * @param _initialWindowSize - A map with width, height, top and left
   * @internal
   */
  constructor(_config, _initialWindowSize, _layoutManager) {
    super();
    this._config = _config;
    this._initialWindowSize = _initialWindowSize;
    this._layoutManager = _layoutManager;
    this._isInitialised = false;
    this._popoutWindow = null;
    this.createWindow();
  }
  toConfig() {
    var _a, _b;
    if (this._isInitialised === false) {
      throw new Error("Can't create config, layout not yet initialised");
    }
    const glInstance = this.getGlInstance();
    const glInstanceConfig = glInstance.saveLayout();
    let left;
    let top;
    if (this._popoutWindow === null) {
      left = null;
      top = null;
    } else {
      left = (_a = this._popoutWindow.screenX) !== null && _a !== void 0 ? _a : this._popoutWindow.screenLeft;
      top = (_b = this._popoutWindow.screenY) !== null && _b !== void 0 ? _b : this._popoutWindow.screenTop;
    }
    const window2 = {
      width: this.getGlInstance().width,
      height: this.getGlInstance().height,
      left,
      top
    };
    const config = {
      root: glInstanceConfig.root,
      openPopouts: glInstanceConfig.openPopouts,
      settings: glInstanceConfig.settings,
      dimensions: glInstanceConfig.dimensions,
      header: glInstanceConfig.header,
      window: window2,
      parentId: this._config.parentId,
      indexInParent: this._config.indexInParent,
      resolved: true
    };
    return config;
  }
  getGlInstance() {
    if (this._popoutWindow === null) {
      throw new UnexpectedNullError("BPGGI24693");
    }
    return this._popoutWindow.__glInstance;
  }
  /**
   * Retrieves the native BrowserWindow backing this popout.
   * Might throw an UnexpectedNullError exception when the window is not initialized yet.
   * @public
   */
  getWindow() {
    if (this._popoutWindow === null) {
      throw new UnexpectedNullError("BPGW087215");
    }
    return this._popoutWindow;
  }
  close() {
    if (this.getGlInstance()) {
      this.getGlInstance().closeWindow();
    } else {
      try {
        this.getWindow().close();
      } catch (e) {
      }
    }
  }
  /**
   * Returns the popped out item to its original position. If the original
   * parent isn't available anymore it falls back to the layout's topmost element
   */
  popIn() {
    let parentItem;
    let index = this._config.indexInParent;
    if (!this._config.parentId) {
      return;
    }
    const glInstanceLayoutConfig = this.getGlInstance().saveLayout();
    const copiedGlInstanceLayoutConfig = deepExtend({}, glInstanceLayoutConfig);
    const copiedRoot = copiedGlInstanceLayoutConfig.root;
    if (copiedRoot === void 0) {
      throw new UnexpectedUndefinedError("BPPIR19998");
    }
    const groundItem = this._layoutManager.groundItem;
    if (groundItem === void 0) {
      throw new UnexpectedUndefinedError("BPPIG34972");
    }
    parentItem = groundItem.getItemsByPopInParentId(this._config.parentId)[0];
    if (!parentItem) {
      if (groundItem.contentItems.length > 0) {
        parentItem = groundItem.contentItems[0];
      } else {
        parentItem = groundItem;
      }
      index = 0;
    }
    const newContentItem = this._layoutManager.createAndInitContentItem(copiedRoot, parentItem);
    parentItem.addChild(newContentItem, index);
    if (this._layoutManager.layoutConfig.settings.popInOnClose) {
      this._onClose();
    } else {
      this.close();
    }
  }
  /**
   * Creates the URL and window parameter
   * and opens a new window
   * @internal
   */
  createWindow() {
    const url = this.createUrl();
    const target = Math.floor(Math.random() * 1e6).toString(36);
    const features = this.serializeWindowFeatures({
      width: this._initialWindowSize.width,
      height: this._initialWindowSize.height,
      innerWidth: this._initialWindowSize.width,
      innerHeight: this._initialWindowSize.height,
      menubar: "no",
      toolbar: "no",
      location: "no",
      personalbar: "no",
      resizable: "yes",
      scrollbars: "no",
      status: "no"
    });
    this._popoutWindow = globalThis.open(url, target, features);
    if (!this._popoutWindow) {
      if (this._layoutManager.layoutConfig.settings.blockedPopoutsThrowError === true) {
        const error = new PopoutBlockedError("Popout blocked");
        throw error;
      } else {
        return;
      }
    }
    this._popoutWindow.addEventListener("load", () => this.positionWindow(), { passive: true });
    this._popoutWindow.addEventListener("beforeunload", () => {
      if (this._layoutManager.layoutConfig.settings.popInOnClose) {
        this.popIn();
      } else {
        this._onClose();
      }
    }, { passive: true });
    this._checkReadyInterval = setInterval(() => this.checkReady(), 10);
  }
  /** @internal */
  checkReady() {
    if (this._popoutWindow === null) {
      throw new UnexpectedNullError("BPCR01844");
    } else {
      if (this._popoutWindow.__glInstance && this._popoutWindow.__glInstance.isInitialised) {
        this.onInitialised();
        if (this._checkReadyInterval !== void 0) {
          clearInterval(this._checkReadyInterval);
          this._checkReadyInterval = void 0;
        }
      }
    }
  }
  /**
   * Serialises a map of key:values to a window options string
   *
   * @param windowOptions -
   *
   * @returns serialised window options
   * @internal
   */
  serializeWindowFeatures(windowOptions) {
    const windowOptionsString = [];
    for (const key in windowOptions) {
      windowOptionsString.push(key + "=" + windowOptions[key].toString());
    }
    return windowOptionsString.join(",");
  }
  /**
   * Creates the URL for the new window, including the
   * config GET parameter
   *
   * @returns URL
   * @internal
   */
  createUrl() {
    const storageKey = "gl-window-config-" + getUniqueId();
    const config = ResolvedLayoutConfig.minifyConfig(this._config);
    try {
      localStorage.setItem(storageKey, JSON.stringify(config));
    } catch (e) {
      throw new Error("Error while writing to localStorage " + getErrorMessage(e));
    }
    const url = new URL(location.href);
    url.searchParams.set("gl-window", storageKey);
    return url.toString();
  }
  /**
   * Move the newly created window roughly to
   * where the component used to be.
   * @internal
   */
  positionWindow() {
    if (this._popoutWindow === null) {
      throw new Error("BrowserPopout.positionWindow: null popoutWindow");
    } else {
      this._popoutWindow.moveTo(this._initialWindowSize.left, this._initialWindowSize.top);
      this._popoutWindow.focus();
    }
  }
  /**
   * Callback when the new window is opened and the GoldenLayout instance
   * within it is initialised
   * @internal
   */
  onInitialised() {
    this._isInitialised = true;
    this.getGlInstance().on("popIn", () => this.popIn());
    this.emit("initialised");
  }
  /**
   * Invoked 50ms after the window unload event
   * @internal
   */
  _onClose() {
    setTimeout(() => this.emit("closed"), 50);
  }
};

// dist/esm/ts/items/content-item.js
var ContentItem = class extends EventEmitter {
  /** @internal */
  constructor(layoutManager, config, _parent, _element) {
    super();
    this.layoutManager = layoutManager;
    this._parent = _parent;
    this._element = _element;
    this._popInParentIds = [];
    this._type = config.type;
    this._id = config.id;
    this._isInitialised = false;
    this.isGround = false;
    this.isRow = false;
    this.isColumn = false;
    this.isStack = false;
    this.isComponent = false;
    this.size = config.size;
    this.sizeUnit = config.sizeUnit;
    this.minSize = config.minSize;
    this.minSizeUnit = config.minSizeUnit;
    this._isClosable = config.isClosable;
    this._pendingEventPropagations = {};
    this._throttledEvents = ["stateChanged"];
    this._contentItems = this.createContentItems(config.content);
  }
  get type() {
    return this._type;
  }
  get id() {
    return this._id;
  }
  set id(value) {
    this._id = value;
  }
  /** @internal */
  get popInParentIds() {
    return this._popInParentIds;
  }
  get parent() {
    return this._parent;
  }
  get contentItems() {
    return this._contentItems;
  }
  get isClosable() {
    return this._isClosable;
  }
  get element() {
    return this._element;
  }
  get isInitialised() {
    return this._isInitialised;
  }
  static isStack(item) {
    return item.isStack;
  }
  static isComponentItem(item) {
    return item.isComponent;
  }
  static isComponentParentableItem(item) {
    return item.isStack || item.isGround;
  }
  /**
   * Removes a child node (and its children) from the tree
   * @param contentItem - The child item to remove
   * @param keepChild - Whether to destroy the removed item
   */
  removeChild(contentItem, keepChild = false) {
    const index = this._contentItems.indexOf(contentItem);
    if (index === -1) {
      throw new Error("Can't remove child item. Unknown content item");
    }
    if (!keepChild) {
      this._contentItems[index].destroy();
    }
    this._contentItems.splice(index, 1);
    if (this._contentItems.length > 0) {
      this.updateSize(false);
    } else {
      if (!this.isGround && this._isClosable === true) {
        if (this._parent === null) {
          throw new UnexpectedNullError("CIUC00874");
        } else {
          this._parent.removeChild(this);
        }
      }
    }
  }
  /**
   * Sets up the tree structure for the newly added child
   * The responsibility for the actual DOM manipulations lies
   * with the concrete item
   *
   * @param contentItem -
   * @param index - If omitted item will be appended
   * @param suspendResize - Used by descendent implementations
   */
  // eslint-disable-next-line @typescript-eslint/no-unused-vars
  addChild(contentItem, index, suspendResize) {
    index !== null && index !== void 0 ? index : index = this._contentItems.length;
    this._contentItems.splice(index, 0, contentItem);
    contentItem.setParent(this);
    if (this._isInitialised === true && contentItem._isInitialised === false) {
      contentItem.init();
    }
    return index;
  }
  /**
   * Replaces oldChild with newChild
   * @param oldChild -
   * @param newChild -
   * @internal
   */
  replaceChild(oldChild, newChild, destroyOldChild = false) {
    const index = this._contentItems.indexOf(oldChild);
    const parentNode = oldChild._element.parentNode;
    if (index === -1) {
      throw new AssertError("CIRCI23232", "Can't replace child. oldChild is not child of this");
    }
    if (parentNode === null) {
      throw new UnexpectedNullError("CIRCP23232");
    } else {
      parentNode.replaceChild(newChild._element, oldChild._element);
      if (destroyOldChild === true) {
        oldChild._parent = null;
        oldChild.destroy();
      }
      this._contentItems[index] = newChild;
      newChild.setParent(this);
      newChild.size = oldChild.size;
      newChild.sizeUnit = oldChild.sizeUnit;
      newChild.minSize = oldChild.minSize;
      newChild.minSizeUnit = oldChild.minSizeUnit;
      if (newChild._parent === null) {
        throw new UnexpectedNullError("CIRCNC45699");
      } else {
        if (newChild._parent._isInitialised === true && newChild._isInitialised === false) {
          newChild.init();
        }
        this.updateSize(false);
      }
    }
  }
  /**
   * Convenience method.
   * Shorthand for this.parent.removeChild( this )
   */
  remove() {
    if (this._parent === null) {
      throw new UnexpectedNullError("CIR11110");
    } else {
      this._parent.removeChild(this);
    }
  }
  /**
   * Removes the component from the layout and creates a new
   * browser window with the component and its children inside
   */
  popout() {
    const parentId = getUniqueId();
    const browserPopout = this.layoutManager.createPopoutFromContentItem(this, void 0, parentId, void 0);
    this.emitBaseBubblingEvent("stateChanged");
    return browserPopout;
  }
  /** @internal */
  calculateConfigContent() {
    const contentItems = this._contentItems;
    const count = contentItems.length;
    const result = new Array(count);
    for (let i = 0; i < count; i++) {
      const item = contentItems[i];
      result[i] = item.toConfig();
    }
    return result;
  }
  /** @internal */
  highlightDropZone(x, y, area) {
    const dropTargetIndicator = this.layoutManager.dropTargetIndicator;
    if (dropTargetIndicator === null) {
      throw new UnexpectedNullError("ACIHDZ5593");
    } else {
      dropTargetIndicator.highlightArea(area, 1);
    }
  }
  /** @internal */
  // eslint-disable-next-line @typescript-eslint/no-unused-vars
  onDrop(contentItem, area) {
    this.addChild(contentItem);
  }
  /** @internal */
  show() {
    this.layoutManager.beginSizeInvalidation();
    try {
      setElementDisplayVisibility(this._element, true);
      for (let i = 0; i < this._contentItems.length; i++) {
        this._contentItems[i].show();
      }
    } finally {
      this.layoutManager.endSizeInvalidation();
    }
  }
  /**
   * Destroys this item ands its children
   * @internal
   */
  destroy() {
    for (let i = 0; i < this._contentItems.length; i++) {
      this._contentItems[i].destroy();
    }
    this._contentItems = [];
    this.emitBaseBubblingEvent("beforeItemDestroyed");
    this._element.remove();
    this.emitBaseBubblingEvent("itemDestroyed");
  }
  /**
   * Returns the area the component currently occupies
   * @internal
   */
  getElementArea(element) {
    element = element !== null && element !== void 0 ? element : this._element;
    const rect = element.getBoundingClientRect();
    const top = rect.top + document.body.scrollTop;
    const left = rect.left + document.body.scrollLeft;
    const width = rect.width;
    const height = rect.height;
    return {
      x1: left,
      y1: top,
      x2: left + width,
      y2: top + height,
      surface: width * height,
      contentItem: this
    };
  }
  /**
   * The tree of content items is created in two steps: First all content items are instantiated,
   * then init is called recursively from top to bottem. This is the basic init function,
   * it can be used, extended or overwritten by the content items
   *
   * Its behaviour depends on the content item
   * @internal
   */
  init() {
    this._isInitialised = true;
    this.emitBaseBubblingEvent("itemCreated");
    this.emitUnknownBubblingEvent(this.type + "Created");
  }
  /** @internal */
  setParent(parent) {
    this._parent = parent;
  }
  /** @internal */
  addPopInParentId(id) {
    if (!this.popInParentIds.includes(id)) {
      this.popInParentIds.push(id);
    }
  }
  /** @internal */
  initContentItems() {
    for (let i = 0; i < this._contentItems.length; i++) {
      this._contentItems[i].init();
    }
  }
  /** @internal */
  hide() {
    this.layoutManager.beginSizeInvalidation();
    try {
      setElementDisplayVisibility(this._element, false);
    } finally {
      this.layoutManager.endSizeInvalidation();
    }
  }
  /** @internal */
  updateContentItemsSize(force) {
    for (let i = 0; i < this._contentItems.length; i++) {
      this._contentItems[i].updateSize(force);
    }
  }
  /**
   * creates all content items for this node at initialisation time
   * PLEASE NOTE, please see addChild for adding contentItems at runtime
   * @internal
   */
  createContentItems(content) {
    const count = content.length;
    const result = new Array(count);
    for (let i = 0; i < content.length; i++) {
      result[i] = this.layoutManager.createContentItem(content[i], this);
    }
    return result;
  }
  /**
   * Called for every event on the item tree. Decides whether the event is a bubbling
   * event and propagates it to its parent
   *
   * @param name - The name of the event
   * @param event -
   * @internal
   */
  propagateEvent(name, args) {
    if (args.length === 1) {
      const event = args[0];
      if (event instanceof EventEmitter.BubblingEvent && event.isPropagationStopped === false && this._isInitialised === true) {
        if (this.isGround === false && this._parent) {
          this._parent.emitUnknown(name, event);
        } else {
          this.scheduleEventPropagationToLayoutManager(name, event);
        }
      }
    }
  }
  tryBubbleEvent(name, args) {
    if (args.length === 1) {
      const event = args[0];
      if (event instanceof EventEmitter.BubblingEvent && event.isPropagationStopped === false && this._isInitialised === true) {
        if (this.isGround === false && this._parent) {
          this._parent.emitUnknown(name, event);
        } else {
          this.scheduleEventPropagationToLayoutManager(name, event);
        }
      }
    }
  }
  /**
   * All raw events bubble up to the Ground element. Some events that
   * are propagated to - and emitted by - the layoutManager however are
   * only string-based, batched and sanitized to make them more usable
   *
   * @param name - The name of the event
   * @internal
   */
  scheduleEventPropagationToLayoutManager(name, event) {
    if (this._throttledEvents.indexOf(name) === -1) {
      this.layoutManager.emitUnknown(name, event);
    } else {
      if (this._pendingEventPropagations[name] !== true) {
        this._pendingEventPropagations[name] = true;
        globalThis.requestAnimationFrame(() => this.propagateEventToLayoutManager(name, event));
      }
    }
  }
  /**
   * Callback for events scheduled by _scheduleEventPropagationToLayoutManager
   *
   * @param name - The name of the event
   * @internal
   */
  propagateEventToLayoutManager(name, event) {
    this._pendingEventPropagations[name] = false;
    this.layoutManager.emitUnknown(name, event);
  }
};

// dist/esm/ts/items/component-item.js
var ComponentItem = class extends ContentItem {
  /** @internal */
  constructor(layoutManager, config, _parentItem) {
    super(layoutManager, config, _parentItem, document.createElement("div"));
    this._parentItem = _parentItem;
    this._focused = false;
    this.isComponent = true;
    this._reorderEnabled = config.reorderEnabled;
    this.applyUpdatableConfig(config);
    this._initialWantMaximise = config.maximised;
    const containerElement = document.createElement("div");
    containerElement.classList.add(
      "lm_content"
      /* Content */
    );
    this.element.appendChild(containerElement);
    this._container = new ComponentContainer(config, this, layoutManager, containerElement, (itemConfig) => this.handleUpdateItemConfigEvent(itemConfig), () => this.show(), () => this.hide(), (suppressEvent) => this.focus(suppressEvent), (suppressEvent) => this.blur(suppressEvent));
  }
  /** @internal @deprecated use {@link (ComponentItem:class).componentType} */
  get componentName() {
    return this._container.componentType;
  }
  get componentType() {
    return this._container.componentType;
  }
  get reorderEnabled() {
    return this._reorderEnabled;
  }
  /** @internal */
  get initialWantMaximise() {
    return this._initialWantMaximise;
  }
  get component() {
    return this._container.component;
  }
  get container() {
    return this._container;
  }
  get parentItem() {
    return this._parentItem;
  }
  get headerConfig() {
    return this._headerConfig;
  }
  get title() {
    return this._title;
  }
  get tab() {
    return this._tab;
  }
  get focused() {
    return this._focused;
  }
  /** @internal */
  destroy() {
    this._container.destroy();
    super.destroy();
  }
  applyUpdatableConfig(config) {
    this.setTitle(config.title);
    this._headerConfig = config.header;
  }
  toConfig() {
    const stateRequestEvent = this._container.stateRequestEvent;
    const state = stateRequestEvent === void 0 ? this._container.state : stateRequestEvent();
    const result = {
      type: ItemType.component,
      content: [],
      size: this.size,
      sizeUnit: this.sizeUnit,
      minSize: this.minSize,
      minSizeUnit: this.minSizeUnit,
      id: this.id,
      maximised: false,
      isClosable: this.isClosable,
      reorderEnabled: this._reorderEnabled,
      title: this._title,
      header: ResolvedHeaderedItemConfig.Header.createCopy(this._headerConfig),
      componentType: ResolvedComponentItemConfig.copyComponentType(this.componentType),
      componentState: state
    };
    return result;
  }
  close() {
    if (this.parent === null) {
      throw new UnexpectedNullError("CIC68883");
    } else {
      this.parent.removeChild(this, false);
    }
  }
  // Used by Drag Proxy
  /** @internal */
  enterDragMode(width, height) {
    setElementWidth(this.element, width);
    setElementHeight(this.element, height);
    this._container.enterDragMode(width, height);
  }
  /** @internal */
  exitDragMode() {
    this._container.exitDragMode();
  }
  /** @internal */
  enterStackMaximised() {
    this._container.enterStackMaximised();
  }
  /** @internal */
  exitStackMaximised() {
    this._container.exitStackMaximised();
  }
  // Used by Drag Proxy
  /** @internal */
  drag() {
    this._container.drag();
  }
  /** @internal */
  updateSize(force) {
    this.updateNodeSize(force);
  }
  /** @internal */
  init() {
    this.updateNodeSize(false);
    super.init();
    this._container.emit("open");
    this.initContentItems();
  }
  /**
   * Set this component's title
   *
   * @public
   * @param title -
   */
  setTitle(title) {
    this._title = title;
    this.emit("titleChanged", title);
    this.emit("stateChanged");
  }
  setTab(tab) {
    this._tab = tab;
    this.emit("tab", tab);
    this._container.setTab(tab);
  }
  /** @internal */
  hide() {
    super.hide();
    this._container.setVisibility(false);
  }
  /** @internal */
  show() {
    super.show();
    this._container.setVisibility(true);
  }
  /**
   * Focuses the item if it is not already focused
   */
  focus(suppressEvent = false) {
    this.parentItem.setActiveComponentItem(this, true, suppressEvent);
  }
  /** @internal */
  setFocused(suppressEvent) {
    this._focused = true;
    this.tab.setFocused();
    if (!suppressEvent) {
      this.emitBaseBubblingEvent("focus");
    }
  }
  /**
   * Blurs (defocuses) the item if it is focused
   */
  blur(suppressEvent = false) {
    if (this._focused) {
      this.layoutManager.setFocusedComponentItem(void 0, suppressEvent);
    }
  }
  /** @internal */
  setBlurred(suppressEvent) {
    this._focused = false;
    this.tab.setBlurred();
    if (!suppressEvent) {
      this.emitBaseBubblingEvent("blur");
    }
  }
  /** @internal */
  setParent(parent) {
    this._parentItem = parent;
    super.setParent(parent);
  }
  /** @internal */
  handleUpdateItemConfigEvent(itemConfig) {
    this.applyUpdatableConfig(itemConfig);
  }
  /** @internal */
  updateNodeSize(force) {
    if (this.element.style.display !== "none") {
      const { width, height } = getElementWidthAndHeight(this.element);
      this._container.setSizeToNodeSize(width, height, force);
    }
  }
};

// dist/esm/ts/items/component-parentable-item.js
var ComponentParentableItem = class extends ContentItem {
  constructor() {
    super(...arguments);
    this._focused = false;
  }
  get focused() {
    return this._focused;
  }
  /** @internal */
  setFocusedValue(value) {
    this._focused = value;
  }
};

// dist/esm/ts/utils/drag-listener.js
var DragListener = class extends EventEmitter {
  constructor(_eElement, extraAllowableChildTargets) {
    super();
    this._eElement = _eElement;
    this._pointerTracking = false;
    this._pointerDownEventListener = (ev) => this.onPointerDown(ev);
    this._pointerMoveEventListener = (ev) => this.onPointerMove(ev);
    this._pointerUpEventListener = (ev) => this.onPointerUp(ev);
    this._timeout = void 0;
    this._allowableTargets = [_eElement, ...extraAllowableChildTargets];
    this._oDocument = document;
    this._eBody = document.body;
    this._nDelay = 1800;
    this._nDistance = 10;
    this._nX = 0;
    this._nY = 0;
    this._nOriginalX = 0;
    this._nOriginalY = 0;
    this._dragging = false;
    this._eElement.addEventListener("pointerdown", this._pointerDownEventListener, { passive: true });
  }
  destroy() {
    this.checkRemovePointerTrackingEventListeners();
    this._eElement.removeEventListener("pointerdown", this._pointerDownEventListener);
  }
  cancelDrag() {
    this.processDragStop(void 0);
  }
  onPointerDown(oEvent) {
    if (this._allowableTargets.includes(oEvent.target) && oEvent.isPrimary) {
      const coordinates = this.getPointerCoordinates(oEvent);
      this.processPointerDown(coordinates);
    }
  }
  processPointerDown(coordinates) {
    this._nOriginalX = coordinates.x;
    this._nOriginalY = coordinates.y;
    this._oDocument.addEventListener("pointermove", this._pointerMoveEventListener);
    this._oDocument.addEventListener("pointerup", this._pointerUpEventListener, { passive: true });
    this._pointerTracking = true;
    this._timeout = setTimeout(() => {
      try {
        this.startDrag();
      } catch (err) {
        console.error(err);
        throw err;
      }
    }, this._nDelay);
  }
  onPointerMove(oEvent) {
    if (this._pointerTracking) {
      this.processDragMove(oEvent);
      oEvent.preventDefault();
    }
  }
  processDragMove(dragEvent) {
    this._nX = dragEvent.pageX - this._nOriginalX;
    this._nY = dragEvent.pageY - this._nOriginalY;
    if (this._dragging === false) {
      if (Math.abs(this._nX) > this._nDistance || Math.abs(this._nY) > this._nDistance) {
        this.startDrag();
      }
    }
    if (this._dragging) {
      this.emit("drag", this._nX, this._nY, dragEvent);
    }
  }
  onPointerUp(oEvent) {
    this.processDragStop(oEvent);
  }
  processDragStop(dragEvent) {
    var _a;
    if (this._timeout !== void 0) {
      clearTimeout(this._timeout);
      this._timeout = void 0;
    }
    this.checkRemovePointerTrackingEventListeners();
    if (this._dragging === true) {
      this._eBody.classList.remove(
        "lm_dragging"
        /* Dragging */
      );
      this._eElement.classList.remove(
        "lm_dragging"
        /* Dragging */
      );
      (_a = this._oDocument.querySelector("iframe")) === null || _a === void 0 ? void 0 : _a.style.setProperty("pointer-events", "");
      this._dragging = false;
      this.emit("dragStop", dragEvent);
    }
  }
  checkRemovePointerTrackingEventListeners() {
    if (this._pointerTracking) {
      this._oDocument.removeEventListener("pointermove", this._pointerMoveEventListener);
      this._oDocument.removeEventListener("pointerup", this._pointerUpEventListener);
      this._pointerTracking = false;
    }
  }
  startDrag() {
    var _a;
    if (this._timeout !== void 0) {
      clearTimeout(this._timeout);
      this._timeout = void 0;
    }
    this._dragging = true;
    this._eBody.classList.add(
      "lm_dragging"
      /* Dragging */
    );
    this._eElement.classList.add(
      "lm_dragging"
      /* Dragging */
    );
    (_a = this._oDocument.querySelector("iframe")) === null || _a === void 0 ? void 0 : _a.style.setProperty("pointer-events", "none");
    this.emit("dragStart", this._nOriginalX, this._nOriginalY);
  }
  getPointerCoordinates(event) {
    const result = {
      x: event.pageX,
      y: event.pageY
    };
    return result;
  }
};

// dist/esm/ts/controls/splitter.js
var Splitter = class {
  constructor(_isVertical, _size, grabSize) {
    this._isVertical = _isVertical;
    this._size = _size;
    this._grabSize = grabSize < this._size ? this._size : grabSize;
    this._element = document.createElement("div");
    this._element.classList.add(
      "lm_splitter"
      /* Splitter */
    );
    const dragHandleElement = document.createElement("div");
    dragHandleElement.classList.add(
      "lm_drag_handle"
      /* DragHandle */
    );
    const handleExcessSize = this._grabSize - this._size;
    const handleExcessPos = handleExcessSize / 2;
    if (this._isVertical) {
      dragHandleElement.style.top = numberToPixels(-handleExcessPos);
      dragHandleElement.style.height = numberToPixels(this._size + handleExcessSize);
      this._element.classList.add(
        "lm_vertical"
        /* Vertical */
      );
      this._element.style.height = numberToPixels(this._size);
    } else {
      dragHandleElement.style.left = numberToPixels(-handleExcessPos);
      dragHandleElement.style.width = numberToPixels(this._size + handleExcessSize);
      this._element.classList.add(
        "lm_horizontal"
        /* Horizontal */
      );
      this._element.style.width = numberToPixels(this._size);
    }
    this._element.appendChild(dragHandleElement);
    this._dragListener = new DragListener(this._element, [dragHandleElement]);
  }
  get element() {
    return this._element;
  }
  destroy() {
    this._element.remove();
  }
  on(eventName, callback) {
    this._dragListener.on(eventName, callback);
  }
};

// dist/esm/ts/items/row-or-column.js
var RowOrColumn = class _RowOrColumn extends ContentItem {
  /** @internal */
  constructor(isColumn, layoutManager, config, _rowOrColumnParent) {
    super(layoutManager, config, _rowOrColumnParent, _RowOrColumn.createElement(document, isColumn));
    this._rowOrColumnParent = _rowOrColumnParent;
    this._splitter = [];
    this.isRow = !isColumn;
    this.isColumn = isColumn;
    this._childElementContainer = this.element;
    this._splitterSize = layoutManager.layoutConfig.dimensions.borderWidth;
    this._splitterGrabSize = layoutManager.layoutConfig.dimensions.borderGrabWidth;
    this._isColumn = isColumn;
    this._dimension = isColumn ? "height" : "width";
    this._splitterPosition = null;
    this._splitterMinPosition = null;
    this._splitterMaxPosition = null;
    switch (config.type) {
      case ItemType.row:
      case ItemType.column:
        this._configType = config.type;
        break;
      default:
        throw new AssertError("ROCCCT00925");
    }
  }
  newComponent(componentType, componentState, title, index) {
    const itemConfig = {
      type: "component",
      componentType,
      componentState,
      title
    };
    return this.newItem(itemConfig, index);
  }
  addComponent(componentType, componentState, title, index) {
    const itemConfig = {
      type: "component",
      componentType,
      componentState,
      title
    };
    return this.addItem(itemConfig, index);
  }
  newItem(itemConfig, index) {
    index = this.addItem(itemConfig, index);
    const createdItem = this.contentItems[index];
    if (ContentItem.isStack(createdItem) && ItemConfig.isComponent(itemConfig)) {
      return createdItem.contentItems[0];
    } else {
      return createdItem;
    }
  }
  addItem(itemConfig, index) {
    this.layoutManager.checkMinimiseMaximisedStack();
    const resolvedItemConfig = ItemConfig.resolve(itemConfig, false);
    const contentItem = this.layoutManager.createAndInitContentItem(resolvedItemConfig, this);
    return this.addChild(contentItem, index, false);
  }
  /**
   * Add a new contentItem to the Row or Column
   *
   * @param contentItem -
   * @param index - The position of the new item within the Row or Column.
   *                If no index is provided the item will be added to the end
   * @param suspendResize - If true the items won't be resized. This will leave the item in
   *                        an inconsistent state and is only intended to be used if multiple
   *                        children need to be added in one go and resize is called afterwards
   *
   * @returns
   */
  addChild(contentItem, index, suspendResize) {
    if (index === void 0) {
      index = this.contentItems.length;
    }
    if (this.contentItems.length > 0) {
      const splitterElement = this.createSplitter(Math.max(0, index - 1)).element;
      if (index > 0) {
        this.contentItems[index - 1].element.insertAdjacentElement("afterend", splitterElement);
        splitterElement.insertAdjacentElement("afterend", contentItem.element);
      } else {
        this.contentItems[0].element.insertAdjacentElement("beforebegin", splitterElement);
        splitterElement.insertAdjacentElement("beforebegin", contentItem.element);
      }
    } else {
      this._childElementContainer.appendChild(contentItem.element);
    }
    super.addChild(contentItem, index);
    const newItemSize = 1 / this.contentItems.length * 100;
    if (suspendResize === true) {
      this.emitBaseBubblingEvent("stateChanged");
      return index;
    }
    for (let i = 0; i < this.contentItems.length; i++) {
      const indexedContentItem = this.contentItems[i];
      if (indexedContentItem === contentItem) {
        contentItem.size = newItemSize;
      } else {
        const itemSize = indexedContentItem.size *= (100 - newItemSize) / 100;
        indexedContentItem.size = itemSize;
      }
    }
    this.updateSize(false);
    this.emitBaseBubblingEvent("stateChanged");
    return index;
  }
  /**
   * Removes a child of this element
   *
   * @param contentItem -
   * @param keepChild - If true the child will be removed, but not destroyed
   *
   */
  removeChild(contentItem, keepChild) {
    const index = this.contentItems.indexOf(contentItem);
    const splitterIndex = Math.max(index - 1, 0);
    if (index === -1) {
      throw new Error("Can't remove child. ContentItem is not child of this Row or Column");
    }
    if (this._splitter[splitterIndex]) {
      this._splitter[splitterIndex].destroy();
      this._splitter.splice(splitterIndex, 1);
    }
    super.removeChild(contentItem, keepChild);
    if (this.contentItems.length === 1 && this.isClosable === true) {
      const childItem = this.contentItems[0];
      this.contentItems.length = 0;
      this._rowOrColumnParent.replaceChild(this, childItem, true);
    } else {
      this.updateSize(false);
      this.emitBaseBubblingEvent("stateChanged");
    }
  }
  /**
   * Replaces a child of this Row or Column with another contentItem
   */
  replaceChild(oldChild, newChild) {
    const size = oldChild.size;
    super.replaceChild(oldChild, newChild);
    newChild.size = size;
    this.updateSize(false);
    this.emitBaseBubblingEvent("stateChanged");
  }
  /**
   * Called whenever the dimensions of this item or one of its parents change
   */
  updateSize(force) {
    this.layoutManager.beginVirtualSizedContainerAdding();
    try {
      this.updateNodeSize();
      this.updateContentItemsSize(force);
    } finally {
      this.layoutManager.endVirtualSizedContainerAdding();
    }
  }
  /**
   * Invoked recursively by the layout manager. ContentItem.init appends
   * the contentItem's DOM elements to the container, RowOrColumn init adds splitters
   * in between them
   * @internal
   */
  init() {
    if (this.isInitialised === true)
      return;
    this.updateNodeSize();
    for (let i = 0; i < this.contentItems.length; i++) {
      this._childElementContainer.appendChild(this.contentItems[i].element);
    }
    super.init();
    for (let i = 0; i < this.contentItems.length - 1; i++) {
      this.contentItems[i].element.insertAdjacentElement("afterend", this.createSplitter(i).element);
    }
    this.initContentItems();
  }
  toConfig() {
    const result = {
      type: this.type,
      content: this.calculateConfigContent(),
      size: this.size,
      sizeUnit: this.sizeUnit,
      minSize: this.minSize,
      minSizeUnit: this.minSizeUnit,
      id: this.id,
      isClosable: this.isClosable
    };
    return result;
  }
  /** @internal */
  setParent(parent) {
    this._rowOrColumnParent = parent;
    super.setParent(parent);
  }
  /** @internal */
  updateNodeSize() {
    if (this.contentItems.length > 0) {
      this.calculateRelativeSizes();
      this.setAbsoluteSizes();
    }
    this.emitBaseBubblingEvent("stateChanged");
    this.emit("resize");
  }
  /**
   * Turns the relative sizes calculated by calculateRelativeSizes into
   * absolute pixel values and applies them to the children's DOM elements
   *
   * Assigns additional pixels to counteract Math.floor
   * @internal
   */
  setAbsoluteSizes() {
    const absoluteSizes = this.calculateAbsoluteSizes();
    for (let i = 0; i < this.contentItems.length; i++) {
      if (absoluteSizes.additionalPixel - i > 0) {
        absoluteSizes.itemSizes[i]++;
      }
      if (this._isColumn) {
        setElementWidth(this.contentItems[i].element, absoluteSizes.crossAxisSize);
        setElementHeight(this.contentItems[i].element, absoluteSizes.itemSizes[i]);
      } else {
        setElementWidth(this.contentItems[i].element, absoluteSizes.itemSizes[i]);
        setElementHeight(this.contentItems[i].element, absoluteSizes.crossAxisSize);
      }
    }
  }
  /**
   * Calculates the absolute sizes of all of the children of this Item.
   * @returns Set with absolute sizes and additional pixels.
   * @internal
   */
  calculateAbsoluteSizes() {
    const totalSplitterSize = (this.contentItems.length - 1) * this._splitterSize;
    const { width: elementWidth, height: elementHeight } = getElementWidthAndHeight(this.element);
    let totalSize;
    let crossAxisSize;
    if (this._isColumn) {
      totalSize = elementHeight - totalSplitterSize;
      crossAxisSize = elementWidth;
    } else {
      totalSize = elementWidth - totalSplitterSize;
      crossAxisSize = elementHeight;
    }
    let totalAssigned = 0;
    const itemSizes = [];
    for (let i = 0; i < this.contentItems.length; i++) {
      const contentItem = this.contentItems[i];
      let itemSize;
      if (contentItem.sizeUnit === SizeUnitEnum.Percent) {
        itemSize = Math.floor(totalSize * (contentItem.size / 100));
      } else {
        throw new AssertError("ROCCAS6692");
      }
      totalAssigned += itemSize;
      itemSizes.push(itemSize);
    }
    const additionalPixel = Math.floor(totalSize - totalAssigned);
    return {
      itemSizes,
      additionalPixel,
      totalSize,
      crossAxisSize
    };
  }
  /**
   * Calculates the relative sizes of all children of this Item. The logic
   * is as follows:
   *
   * - Add up the total size of all items that have a configured size
   *
   * - If the total == 100 (check for floating point errors)
   *        Excellent, job done
   *
   * - If the total is \> 100,
   *        set the size of items without set dimensions to 1/3 and add this to the total
   *        set the size off all items so that the total is hundred relative to their original size
   *
   * - If the total is \< 100
   *        If there are items without set dimensions, distribute the remainder to 100 evenly between them
   *        If there are no items without set dimensions, increase all items sizes relative to
   *        their original size so that they add up to 100
   *
   * @internal
   */
  calculateRelativeSizes() {
    let total = 0;
    const itemsWithFractionalSize = [];
    let totalFractionalSize = 0;
    for (let i = 0; i < this.contentItems.length; i++) {
      const contentItem = this.contentItems[i];
      const sizeUnit = contentItem.sizeUnit;
      switch (sizeUnit) {
        case SizeUnitEnum.Percent: {
          total += contentItem.size;
          break;
        }
        case SizeUnitEnum.Fractional: {
          itemsWithFractionalSize.push(contentItem);
          totalFractionalSize += contentItem.size;
          break;
        }
        default:
          throw new AssertError("ROCCRS49110", JSON.stringify(contentItem));
      }
    }
    if (Math.round(total) === 100) {
      this.respectMinItemSize();
      return;
    } else {
      if (Math.round(total) < 100 && itemsWithFractionalSize.length > 0) {
        const fractionalAllocatedSize = 100 - total;
        for (let i = 0; i < itemsWithFractionalSize.length; i++) {
          const contentItem = itemsWithFractionalSize[i];
          contentItem.size = fractionalAllocatedSize * (contentItem.size / totalFractionalSize);
          contentItem.sizeUnit = SizeUnitEnum.Percent;
        }
        this.respectMinItemSize();
        return;
      } else {
        if (Math.round(total) > 100 && itemsWithFractionalSize.length > 0) {
          for (let i = 0; i < itemsWithFractionalSize.length; i++) {
            const contentItem = itemsWithFractionalSize[i];
            contentItem.size = 50 * (contentItem.size / totalFractionalSize);
            contentItem.sizeUnit = SizeUnitEnum.Percent;
          }
          total += 50;
        }
        for (let i = 0; i < this.contentItems.length; i++) {
          const contentItem = this.contentItems[i];
          contentItem.size = contentItem.size / total * 100;
        }
        this.respectMinItemSize();
      }
    }
  }
  /**
   * Adjusts the column widths to respect the dimensions minItemWidth if set.
   * @internal
   */
  respectMinItemSize() {
    const minItemSize = this.calculateContentItemMinSize(this);
    if (minItemSize <= 0 || this.contentItems.length <= 1) {
      return;
    } else {
      let totalOverMin = 0;
      let totalUnderMin = 0;
      const entriesOverMin = [];
      const allEntries = [];
      const absoluteSizes = this.calculateAbsoluteSizes();
      for (let i = 0; i < absoluteSizes.itemSizes.length; i++) {
        const itemSize = absoluteSizes.itemSizes[i];
        let entry;
        if (itemSize < minItemSize) {
          totalUnderMin += minItemSize - itemSize;
          entry = {
            size: minItemSize
          };
        } else {
          totalOverMin += itemSize - minItemSize;
          entry = {
            size: itemSize
          };
          entriesOverMin.push(entry);
        }
        allEntries.push(entry);
      }
      if (totalUnderMin === 0 || totalUnderMin > totalOverMin) {
        return;
      } else {
        const reducePercent = totalUnderMin / totalOverMin;
        let remainingSize = totalUnderMin;
        for (let i = 0; i < entriesOverMin.length; i++) {
          const entry = entriesOverMin[i];
          const reducedSize = Math.round((entry.size - minItemSize) * reducePercent);
          remainingSize -= reducedSize;
          entry.size -= reducedSize;
        }
        if (remainingSize !== 0) {
          allEntries[allEntries.length - 1].size -= remainingSize;
        }
        for (let i = 0; i < this.contentItems.length; i++) {
          const contentItem = this.contentItems[i];
          contentItem.size = allEntries[i].size / absoluteSizes.totalSize * 100;
        }
      }
    }
  }
  /**
   * Instantiates a new Splitter, binds events to it and adds
   * it to the array of splitters at the position specified as the index argument
   *
   * What it doesn't do though is append the splitter to the DOM
   *
   * @param index - The position of the splitter
   *
   * @returns
   * @internal
   */
  createSplitter(index) {
    const splitter = new Splitter(this._isColumn, this._splitterSize, this._splitterGrabSize);
    splitter.on("drag", (offsetX, offsetY) => this.onSplitterDrag(splitter, offsetX, offsetY));
    splitter.on("dragStop", () => this.onSplitterDragStop(splitter));
    splitter.on("dragStart", () => this.onSplitterDragStart(splitter));
    this._splitter.splice(index, 0, splitter);
    return splitter;
  }
  /**
   * Locates the instance of Splitter in the array of
   * registered splitters and returns a map containing the contentItem
   * before and after the splitters, both of which are affected if the
   * splitter is moved
   *
   * @returns A map of contentItems that the splitter affects
   * @internal
   */
  getSplitItems(splitter) {
    const index = this._splitter.indexOf(splitter);
    return {
      before: this.contentItems[index],
      after: this.contentItems[index + 1]
    };
  }
  calculateContentItemMinSize(contentItem) {
    const minSize = contentItem.minSize;
    if (minSize !== void 0) {
      if (contentItem.minSizeUnit === SizeUnitEnum.Pixel) {
        return minSize;
      } else {
        throw new AssertError("ROCGMD98831", JSON.stringify(contentItem));
      }
    } else {
      const dimensions = this.layoutManager.layoutConfig.dimensions;
      return this._isColumn ? dimensions.defaultMinItemHeight : dimensions.defaultMinItemWidth;
    }
  }
  /**
   * Gets the minimum dimensions for the given item configuration array
   * @internal
   */
  calculateContentItemsTotalMinSize(contentItems) {
    let totalMinSize = 0;
    for (const contentItem of contentItems) {
      totalMinSize += this.calculateContentItemMinSize(contentItem);
    }
    return totalMinSize;
  }
  /**
   * Invoked when a splitter's dragListener fires dragStart. Calculates the splitters
   * movement area once (so that it doesn't need calculating on every mousemove event)
   * @internal
   */
  onSplitterDragStart(splitter) {
    const items = this.getSplitItems(splitter);
    const beforeWidth = pixelsToNumber(items.before.element.style[this._dimension]);
    const afterSize = pixelsToNumber(items.after.element.style[this._dimension]);
    const beforeMinSize = this.calculateContentItemsTotalMinSize(items.before.contentItems);
    const afterMinSize = this.calculateContentItemsTotalMinSize(items.after.contentItems);
    this._splitterPosition = 0;
    this._splitterMinPosition = -1 * (beforeWidth - beforeMinSize);
    this._splitterMaxPosition = afterSize - afterMinSize;
  }
  /**
   * Invoked when a splitter's DragListener fires drag. Updates the splitter's DOM position,
   * but not the sizes of the elements the splitter controls in order to minimize resize events
   *
   * @param splitter -
   * @param offsetX - Relative pixel values to the splitter's original position. Can be negative
   * @param offsetY - Relative pixel values to the splitter's original position. Can be negative
   * @internal
   */
  onSplitterDrag(splitter, offsetX, offsetY) {
    let offset = this._isColumn ? offsetY : offsetX;
    if (this._splitterMinPosition === null || this._splitterMaxPosition === null) {
      throw new UnexpectedNullError("ROCOSD59226");
    }
    offset = Math.max(offset, this._splitterMinPosition);
    offset = Math.min(offset, this._splitterMaxPosition);
    this._splitterPosition = offset;
    const offsetPixels = numberToPixels(offset);
    if (this._isColumn) {
      splitter.element.style.top = offsetPixels;
    } else {
      splitter.element.style.left = offsetPixels;
    }
  }
  /**
   * Invoked when a splitter's DragListener fires dragStop. Resets the splitters DOM position,
   * and applies the new sizes to the elements before and after the splitter and their children
   * on the next animation frame
   * @internal
   */
  onSplitterDragStop(splitter) {
    if (this._splitterPosition === null) {
      throw new UnexpectedNullError("ROCOSDS66932");
    } else {
      const items = this.getSplitItems(splitter);
      const sizeBefore = pixelsToNumber(items.before.element.style[this._dimension]);
      const sizeAfter = pixelsToNumber(items.after.element.style[this._dimension]);
      const splitterPositionInRange = (this._splitterPosition + sizeBefore) / (sizeBefore + sizeAfter);
      const totalRelativeSize = items.before.size + items.after.size;
      items.before.size = splitterPositionInRange * totalRelativeSize;
      items.after.size = (1 - splitterPositionInRange) * totalRelativeSize;
      splitter.element.style.top = numberToPixels(0);
      splitter.element.style.left = numberToPixels(0);
      globalThis.requestAnimationFrame(() => this.updateSize(false));
    }
  }
};
(function(RowOrColumn2) {
  function getElementDimensionSize(element, dimension) {
    if (dimension === "width") {
      return getElementWidth(element);
    } else {
      return getElementHeight(element);
    }
  }
  RowOrColumn2.getElementDimensionSize = getElementDimensionSize;
  function setElementDimensionSize(element, dimension, value) {
    if (dimension === "width") {
      return setElementWidth(element, value);
    } else {
      return setElementHeight(element, value);
    }
  }
  RowOrColumn2.setElementDimensionSize = setElementDimensionSize;
  function createElement(document2, isColumn) {
    const element = document2.createElement("div");
    element.classList.add(
      "lm_item"
      /* Item */
    );
    if (isColumn) {
      element.classList.add(
        "lm_column"
        /* Column */
      );
    } else {
      element.classList.add(
        "lm_row"
        /* Row */
      );
    }
    return element;
  }
  RowOrColumn2.createElement = createElement;
})(RowOrColumn || (RowOrColumn = {}));

// dist/esm/ts/items/ground-item.js
var GroundItem = class _GroundItem extends ComponentParentableItem {
  constructor(layoutManager, rootItemConfig, containerElement) {
    super(layoutManager, ResolvedGroundItemConfig.create(rootItemConfig), null, _GroundItem.createElement(document));
    this.isGround = true;
    this._childElementContainer = this.element;
    this._containerElement = containerElement;
    let before = null;
    while (true) {
      const prev = before ? before.previousSibling : this._containerElement.lastChild;
      if (prev instanceof Element && prev.classList.contains(
        "lm_content"
        /* Content */
      )) {
        before = prev;
      } else {
        break;
      }
    }
    this._containerElement.insertBefore(this.element, before);
  }
  init() {
    if (this.isInitialised === true)
      return;
    this.updateNodeSize();
    for (let i = 0; i < this.contentItems.length; i++) {
      this._childElementContainer.appendChild(this.contentItems[i].element);
    }
    super.init();
    this.initContentItems();
  }
  /**
   * Loads a new Layout
   * Internal only.  To load a new layout with API, use {@link (LayoutManager:class).loadLayout}
   */
  loadRoot(rootItemConfig) {
    this.clearRoot();
    if (rootItemConfig !== void 0) {
      const rootContentItem = this.layoutManager.createAndInitContentItem(rootItemConfig, this);
      this.addChild(rootContentItem, 0);
    }
  }
  clearRoot() {
    const contentItems = this.contentItems;
    switch (contentItems.length) {
      case 0: {
        return;
      }
      case 1: {
        const existingRootContentItem = contentItems[0];
        existingRootContentItem.remove();
        return;
      }
      default: {
        throw new AssertError("GILR07721");
      }
    }
  }
  /**
   * Adds a ContentItem child to root ContentItem.
   * Internal only.  To load a add with API, use {@link (LayoutManager:class).addItem}
   * @returns -1 if added as root otherwise index in root ContentItem's content
   */
  addItem(itemConfig, index) {
    this.layoutManager.checkMinimiseMaximisedStack();
    const resolvedItemConfig = ItemConfig.resolve(itemConfig, false);
    let parent;
    if (this.contentItems.length > 0) {
      parent = this.contentItems[0];
    } else {
      parent = this;
    }
    if (parent.isComponent) {
      throw new Error("Cannot add item as child to ComponentItem");
    } else {
      const contentItem = this.layoutManager.createAndInitContentItem(resolvedItemConfig, parent);
      index = parent.addChild(contentItem, index);
      return parent === this ? -1 : index;
    }
  }
  loadComponentAsRoot(itemConfig) {
    this.clearRoot();
    const resolvedItemConfig = ItemConfig.resolve(itemConfig, false);
    if (resolvedItemConfig.maximised) {
      throw new Error("Root Component cannot be maximised");
    } else {
      const rootContentItem = new ComponentItem(this.layoutManager, resolvedItemConfig, this);
      rootContentItem.init();
      this.addChild(rootContentItem, 0);
    }
  }
  /**
   * Adds a Root ContentItem.
   * Internal only.  To replace Root ContentItem with API, use {@link (LayoutManager:class).loadLayout}
   */
  addChild(contentItem, index) {
    if (this.contentItems.length > 0) {
      throw new Error("Ground node can only have a single child");
    } else {
      this._childElementContainer.appendChild(contentItem.element);
      index = super.addChild(contentItem, index);
      this.updateSize(false);
      this.emitBaseBubblingEvent("stateChanged");
      return index;
    }
  }
  /** @internal */
  calculateConfigContent() {
    const contentItems = this.contentItems;
    const count = contentItems.length;
    const result = new Array(count);
    for (let i = 0; i < count; i++) {
      const item = contentItems[i];
      const itemConfig = item.toConfig();
      if (ResolvedRootItemConfig.isRootItemConfig(itemConfig)) {
        result[i] = itemConfig;
      } else {
        throw new AssertError("RCCC66832");
      }
    }
    return result;
  }
  /** @internal */
  setSize(width, height) {
    if (width === void 0 || height === void 0) {
      this.updateSize(false);
    } else {
      setElementWidth(this.element, width);
      setElementHeight(this.element, height);
      if (this.contentItems.length > 0) {
        setElementWidth(this.contentItems[0].element, width);
        setElementHeight(this.contentItems[0].element, height);
      }
      this.updateContentItemsSize(false);
    }
  }
  /**
   * Adds a Root ContentItem.
   * Internal only.  To replace Root ContentItem with API, use {@link (LayoutManager:class).updateRootSize}
   */
  updateSize(force) {
    this.layoutManager.beginVirtualSizedContainerAdding();
    try {
      this.updateNodeSize();
      this.updateContentItemsSize(force);
    } finally {
      this.layoutManager.endVirtualSizedContainerAdding();
    }
  }
  createSideAreas() {
    const areaSize = 50;
    const oppositeSides = _GroundItem.Area.oppositeSides;
    const result = new Array(Object.keys(oppositeSides).length);
    let idx = 0;
    for (const key in oppositeSides) {
      const side = key;
      const area = this.getElementArea();
      if (area === null) {
        throw new UnexpectedNullError("RCSA77553");
      } else {
        area.side = side;
        if (oppositeSides[side][1] === "2")
          area[side] = area[oppositeSides[side]] - areaSize;
        else
          area[side] = area[oppositeSides[side]] + areaSize;
        area.surface = (area.x2 - area.x1) * (area.y2 - area.y1);
        result[idx++] = area;
      }
    }
    return result;
  }
  highlightDropZone(x, y, area) {
    this.layoutManager.tabDropPlaceholder.remove();
    super.highlightDropZone(x, y, area);
  }
  onDrop(contentItem, area) {
    if (contentItem.isComponent) {
      const itemConfig = ResolvedStackItemConfig.createDefault();
      const component = contentItem;
      itemConfig.header = ResolvedHeaderedItemConfig.Header.createCopy(component.headerConfig);
      const stack = this.layoutManager.createAndInitContentItem(itemConfig, this);
      stack.addChild(contentItem);
      contentItem = stack;
    }
    if (this.contentItems.length === 0) {
      this.addChild(contentItem);
    } else {
      if (contentItem.type === ItemType.row || contentItem.type === ItemType.column) {
        const itemConfig = ResolvedStackItemConfig.createDefault();
        const stack = this.layoutManager.createContentItem(itemConfig, this);
        stack.addChild(contentItem);
        contentItem = stack;
      }
      const type = area.side[0] == "x" ? ItemType.row : ItemType.column;
      const insertBefore = area.side[1] == "2";
      const column = this.contentItems[0];
      if (!(column instanceof RowOrColumn) || column.type !== type) {
        const itemConfig = ResolvedItemConfig.createDefault(type);
        const rowOrColumn = this.layoutManager.createContentItem(itemConfig, this);
        this.replaceChild(column, rowOrColumn);
        rowOrColumn.addChild(contentItem, insertBefore ? 0 : void 0, true);
        rowOrColumn.addChild(column, insertBefore ? void 0 : 0, true);
        column.size = 50;
        contentItem.size = 50;
        contentItem.sizeUnit = SizeUnitEnum.Percent;
        rowOrColumn.updateSize(false);
      } else {
        const sibling = column.contentItems[insertBefore ? 0 : column.contentItems.length - 1];
        column.addChild(contentItem, insertBefore ? 0 : void 0, true);
        sibling.size *= 0.5;
        contentItem.size = sibling.size;
        contentItem.sizeUnit = SizeUnitEnum.Percent;
        column.updateSize(false);
      }
    }
  }
  // No ContentItem can dock with groundItem.  However Stack can have a GroundItem parent and Stack requires that
  // its parent implement dock() function.  Accordingly this function is implemented but throws an exception as it should
  // never be called
  dock() {
    throw new AssertError("GID87731");
  }
  // No ContentItem can dock with groundItem.  However Stack can have a GroundItem parent and Stack requires that
  // its parent implement validateDocking() function.  Accordingly this function is implemented but throws an exception as it should
  // never be called
  validateDocking() {
    throw new AssertError("GIVD87732");
  }
  getAllContentItems() {
    const result = [this];
    this.deepGetAllContentItems(this.contentItems, result);
    return result;
  }
  getConfigMaximisedItems() {
    const result = [];
    this.deepFilterContentItems(this.contentItems, result, (item) => {
      if (ContentItem.isStack(item) && item.initialWantMaximise) {
        return true;
      } else {
        if (ContentItem.isComponentItem(item) && item.initialWantMaximise) {
          return true;
        } else {
          return false;
        }
      }
    });
    return result;
  }
  getItemsByPopInParentId(popInParentId) {
    const result = [];
    this.deepFilterContentItems(this.contentItems, result, (item) => item.popInParentIds.includes(popInParentId));
    return result;
  }
  toConfig() {
    throw new Error("Cannot generate GroundItem config");
  }
  // eslint-disable-next-line @typescript-eslint/no-unused-vars
  setActiveComponentItem(item, focus, suppressFocusEvent) {
  }
  updateNodeSize() {
    const { width, height } = getElementWidthAndHeight(this._containerElement);
    setElementWidth(this.element, width);
    setElementHeight(this.element, height);
    if (this.contentItems.length > 0) {
      setElementWidth(this.contentItems[0].element, width);
      setElementHeight(this.contentItems[0].element, height);
    }
  }
  deepGetAllContentItems(content, result) {
    for (let i = 0; i < content.length; i++) {
      const contentItem = content[i];
      result.push(contentItem);
      this.deepGetAllContentItems(contentItem.contentItems, result);
    }
  }
  deepFilterContentItems(content, result, checkAcceptFtn) {
    for (let i = 0; i < content.length; i++) {
      const contentItem = content[i];
      if (checkAcceptFtn(contentItem)) {
        result.push(contentItem);
      }
      this.deepFilterContentItems(contentItem.contentItems, result, checkAcceptFtn);
    }
  }
};
(function(GroundItem2) {
  let Area;
  (function(Area2) {
    Area2.oppositeSides = {
      y2: "y1",
      x2: "x1",
      y1: "y2",
      x1: "x2"
    };
  })(Area = GroundItem2.Area || (GroundItem2.Area = {}));
  function createElement(document2) {
    const element = document2.createElement("div");
    element.classList.add(
      "lm_goldenlayout"
      /* GoldenLayout */
    );
    element.classList.add(
      "lm_item"
      /* Item */
    );
    element.classList.add(
      "lm_root"
      /* Root */
    );
    return element;
  }
  GroundItem2.createElement = createElement;
})(GroundItem || (GroundItem = {}));

// dist/esm/ts/controls/header-button.js
var HeaderButton = class {
  constructor(_header, label, cssClass, _pushEvent) {
    this._header = _header;
    this._pushEvent = _pushEvent;
    this._clickEventListener = (ev) => this.onClick(ev);
    this._touchStartEventListener = (ev) => this.onTouchStart(ev);
    this._element = document.createElement("div");
    this._element.classList.add(cssClass);
    this._element.title = label;
    this._header.on("destroy", () => this.destroy());
    this._element.addEventListener("click", this._clickEventListener, { passive: true });
    this._element.addEventListener("touchstart", this._touchStartEventListener, { passive: true });
    this._header.controlsContainerElement.appendChild(this._element);
  }
  get element() {
    return this._element;
  }
  destroy() {
    var _a;
    this._element.removeEventListener("click", this._clickEventListener);
    this._element.removeEventListener("touchstart", this._touchStartEventListener);
    (_a = this._element.parentNode) === null || _a === void 0 ? void 0 : _a.removeChild(this._element);
  }
  onClick(ev) {
    this._pushEvent(ev);
  }
  onTouchStart(ev) {
    this._pushEvent(ev);
  }
};

// dist/esm/ts/controls/tab.js
var Tab = class {
  /** @internal */
  constructor(_layoutManager, _componentItem, _closeEvent, _focusEvent, _dragStartEvent) {
    var _a;
    this._layoutManager = _layoutManager;
    this._componentItem = _componentItem;
    this._closeEvent = _closeEvent;
    this._focusEvent = _focusEvent;
    this._dragStartEvent = _dragStartEvent;
    this._isActive = false;
    this._tabClickListener = (ev) => this.onTabClickDown(ev);
    this._tabTouchStartListener = (ev) => this.onTabTouchStart(ev);
    this._closeClickListener = () => this.onCloseClick();
    this._closeTouchStartListener = () => this.onCloseTouchStart();
    this._dragStartListener = (x, y) => this.onDragStart(x, y);
    this._contentItemDestroyListener = () => this.onContentItemDestroy();
    this._tabTitleChangedListener = (title) => this.setTitle(title);
    this._element = document.createElement("div");
    this._element.classList.add(
      "lm_tab"
      /* Tab */
    );
    this._titleElement = document.createElement("span");
    this._titleElement.classList.add(
      "lm_title"
      /* Title */
    );
    this._closeElement = document.createElement("div");
    this._closeElement.classList.add(
      "lm_close_tab"
      /* CloseTab */
    );
    this._element.appendChild(this._titleElement);
    this._element.appendChild(this._closeElement);
    if (_componentItem.isClosable) {
      this._closeElement.style.display = "";
    } else {
      this._closeElement.style.display = "none";
    }
    this.setTitle(_componentItem.title);
    this._componentItem.on("titleChanged", this._tabTitleChangedListener);
    const reorderEnabled = (_a = _componentItem.reorderEnabled) !== null && _a !== void 0 ? _a : this._layoutManager.layoutConfig.settings.reorderEnabled;
    if (reorderEnabled) {
      this.enableReorder();
    }
    this._element.addEventListener("click", this._tabClickListener, { passive: true });
    this._element.addEventListener("touchstart", this._tabTouchStartListener, { passive: true });
    if (this._componentItem.isClosable) {
      this._closeElement.addEventListener("click", this._closeClickListener, { passive: true });
      this._closeElement.addEventListener("touchstart", this._closeTouchStartListener, { passive: true });
    } else {
      this._closeElement.remove();
      this._closeElement = void 0;
    }
    this._componentItem.setTab(this);
    this._layoutManager.emit("tabCreated", this);
  }
  get isActive() {
    return this._isActive;
  }
  // get header(): Header { return this._header; }
  get componentItem() {
    return this._componentItem;
  }
  /** @deprecated use {@link (Tab:class).componentItem} */
  get contentItem() {
    return this._componentItem;
  }
  get element() {
    return this._element;
  }
  get titleElement() {
    return this._titleElement;
  }
  get closeElement() {
    return this._closeElement;
  }
  get reorderEnabled() {
    return this._dragListener !== void 0;
  }
  set reorderEnabled(value) {
    if (value !== this.reorderEnabled) {
      if (value) {
        this.enableReorder();
      } else {
        this.disableReorder();
      }
    }
  }
  /**
   * Sets the tab's title to the provided string and sets
   * its title attribute to a pure text representation (without
   * html tags) of the same string.
   */
  setTitle(title) {
    this._titleElement.innerText = title;
    this._element.title = title;
  }
  /**
   * Sets this tab's active state. To programmatically
   * switch tabs, use Stack.setActiveComponentItem( item ) instead.
   */
  setActive(isActive) {
    if (isActive === this._isActive) {
      return;
    }
    this._isActive = isActive;
    if (isActive) {
      this._element.classList.add(
        "lm_active"
        /* Active */
      );
    } else {
      this._element.classList.remove(
        "lm_active"
        /* Active */
      );
    }
  }
  /**
   * Destroys the tab
   * @internal
   */
  destroy() {
    var _a, _b;
    this._closeEvent = void 0;
    this._focusEvent = void 0;
    this._dragStartEvent = void 0;
    this._element.removeEventListener("click", this._tabClickListener);
    this._element.removeEventListener("touchstart", this._tabTouchStartListener);
    (_a = this._closeElement) === null || _a === void 0 ? void 0 : _a.removeEventListener("click", this._closeClickListener);
    (_b = this._closeElement) === null || _b === void 0 ? void 0 : _b.removeEventListener("touchstart", this._closeTouchStartListener);
    this._componentItem.off("titleChanged", this._tabTitleChangedListener);
    if (this.reorderEnabled) {
      this.disableReorder();
    }
    this._element.remove();
  }
  /** @internal */
  setBlurred() {
    this._element.classList.remove(
      "lm_focused"
      /* Focused */
    );
    this._titleElement.classList.remove(
      "lm_focused"
      /* Focused */
    );
  }
  /** @internal */
  setFocused() {
    this._element.classList.add(
      "lm_focused"
      /* Focused */
    );
    this._titleElement.classList.add(
      "lm_focused"
      /* Focused */
    );
  }
  /**
   * Callback for the DragListener
   * @param x - The tabs absolute x position
   * @param y - The tabs absolute y position
   * @internal
   */
  onDragStart(x, y) {
    if (this._dragListener === void 0) {
      throw new UnexpectedUndefinedError("TODSDLU10093");
    } else {
      if (this._dragStartEvent === void 0) {
        throw new UnexpectedUndefinedError("TODS23309");
      } else {
        this._dragStartEvent(x, y, this._dragListener, this.componentItem);
      }
    }
  }
  /** @internal */
  onContentItemDestroy() {
    if (this._dragListener !== void 0) {
      this._dragListener.destroy();
      this._dragListener = void 0;
    }
  }
  /**
   * Callback when the tab is clicked
   * @internal
   */
  onTabClickDown(event) {
    const target = event.target;
    if (target === this._element || target === this._titleElement) {
      if (event.button === 0) {
        this.notifyFocus();
      } else if (event.button === 1 && this._componentItem.isClosable) {
        this.notifyClose();
      }
    }
  }
  /** @internal */
  onTabTouchStart(event) {
    if (event.target === this._element) {
      this.notifyFocus();
    }
  }
  /**
   * Callback when the tab's close button is clicked
   * @internal
   */
  onCloseClick() {
    this.notifyClose();
  }
  /** @internal */
  onCloseTouchStart() {
    this.notifyClose();
  }
  /**
   * Callback to capture tab close button mousedown
   * to prevent tab from activating.
   * @internal
   */
  // private onCloseMousedown(): void {
  //     // event.stopPropagation();
  // }
  /** @internal */
  notifyClose() {
    if (this._closeEvent === void 0) {
      throw new UnexpectedUndefinedError("TNC15007");
    } else {
      this._closeEvent(this._componentItem);
    }
  }
  /** @internal */
  notifyFocus() {
    if (this._focusEvent === void 0) {
      throw new UnexpectedUndefinedError("TNA15007");
    } else {
      this._focusEvent(this._componentItem);
    }
  }
  /** @internal */
  enableReorder() {
    this._dragListener = new DragListener(this._element, [this._titleElement]);
    this._dragListener.on("dragStart", this._dragStartListener);
    this._componentItem.on("destroy", this._contentItemDestroyListener);
  }
  /** @internal */
  disableReorder() {
    if (this._dragListener === void 0) {
      throw new UnexpectedUndefinedError("TDR87745");
    } else {
      this._componentItem.off("destroy", this._contentItemDestroyListener);
      this._dragListener.off("dragStart", this._dragStartListener);
      this._dragListener = void 0;
    }
  }
};

// dist/esm/ts/controls/tabs-container.js
var TabsContainer = class {
  constructor(_layoutManager, _componentRemoveEvent, _componentFocusEvent, _componentDragStartEvent, _dropdownActiveChangedEvent) {
    this._layoutManager = _layoutManager;
    this._componentRemoveEvent = _componentRemoveEvent;
    this._componentFocusEvent = _componentFocusEvent;
    this._componentDragStartEvent = _componentDragStartEvent;
    this._dropdownActiveChangedEvent = _dropdownActiveChangedEvent;
    this._tabs = [];
    this._lastVisibleTabIndex = -1;
    this._dropdownActive = false;
    this._element = document.createElement("section");
    this._element.classList.add(
      "lm_tabs"
      /* Tabs */
    );
    this._dropdownElement = document.createElement("section");
    this._dropdownElement.classList.add(
      "lm_tabdropdown_list"
      /* TabDropdownList */
    );
    this._dropdownElement.style.display = "none";
  }
  get tabs() {
    return this._tabs;
  }
  get tabCount() {
    return this._tabs.length;
  }
  get lastVisibleTabIndex() {
    return this._lastVisibleTabIndex;
  }
  get element() {
    return this._element;
  }
  get dropdownElement() {
    return this._dropdownElement;
  }
  get dropdownActive() {
    return this._dropdownActive;
  }
  destroy() {
    for (let i = 0; i < this._tabs.length; i++) {
      this._tabs[i].destroy();
    }
  }
  /**
   * Creates a new tab and associates it with a contentItem
   * @param index - The position of the tab
   */
  createTab(componentItem, index) {
    for (let i = 0; i < this._tabs.length; i++) {
      if (this._tabs[i].componentItem === componentItem) {
        return;
      }
    }
    const tab = new Tab(this._layoutManager, componentItem, (item) => this.handleTabCloseEvent(item), (item) => this.handleTabFocusEvent(item), (x, y, dragListener, item) => this.handleTabDragStartEvent(x, y, dragListener, item));
    if (index === void 0) {
      index = this._tabs.length;
    }
    this._tabs.splice(index, 0, tab);
    if (index < this._element.childNodes.length) {
      this._element.insertBefore(tab.element, this._element.childNodes[index]);
    } else {
      this._element.appendChild(tab.element);
    }
  }
  removeTab(componentItem) {
    for (let i = 0; i < this._tabs.length; i++) {
      if (this._tabs[i].componentItem === componentItem) {
        const tab = this._tabs[i];
        tab.destroy();
        this._tabs.splice(i, 1);
        return;
      }
    }
    throw new Error("contentItem is not controlled by this header");
  }
  processActiveComponentChanged(newActiveComponentItem) {
    let activeIndex = -1;
    for (let i = 0; i < this._tabs.length; i++) {
      const isActive = this._tabs[i].componentItem === newActiveComponentItem;
      this._tabs[i].setActive(isActive);
      if (isActive) {
        activeIndex = i;
      }
    }
    if (activeIndex < 0) {
      throw new AssertError("HSACI56632");
    } else {
      if (this._layoutManager.layoutConfig.settings.reorderOnTabMenuClick) {
        if (this._lastVisibleTabIndex !== -1 && activeIndex > this._lastVisibleTabIndex) {
          const activeTab = this._tabs[activeIndex];
          for (let j = activeIndex; j > 0; j--) {
            this._tabs[j] = this._tabs[j - 1];
          }
          this._tabs[0] = activeTab;
        }
      }
    }
  }
  /**
   * Pushes the tabs to the tab dropdown if the available space is not sufficient
   */
  updateTabSizes(availableWidth, activeComponentItem) {
    let dropDownActive = false;
    const success = this.tryUpdateTabSizes(dropDownActive, availableWidth, activeComponentItem);
    if (!success) {
      dropDownActive = true;
      this.tryUpdateTabSizes(dropDownActive, availableWidth, activeComponentItem);
    }
    if (dropDownActive !== this._dropdownActive) {
      this._dropdownActive = dropDownActive;
      this._dropdownActiveChangedEvent();
    }
  }
  tryUpdateTabSizes(dropdownActive, availableWidth, activeComponentItem) {
    if (this._tabs.length > 0) {
      if (activeComponentItem === void 0) {
        throw new Error("non-empty tabs must have active component item");
      }
      let cumulativeTabWidth = 0;
      let tabOverlapAllowanceExceeded = false;
      const tabOverlapAllowance = this._layoutManager.layoutConfig.settings.tabOverlapAllowance;
      const activeIndex = this._tabs.indexOf(activeComponentItem.tab);
      const activeTab = this._tabs[activeIndex];
      this._lastVisibleTabIndex = -1;
      for (let i = 0; i < this._tabs.length; i++) {
        const tabElement = this._tabs[i].element;
        if (tabElement.parentElement !== this._element) {
          this._element.appendChild(tabElement);
        }
        const tabMarginRightPixels = getComputedStyle(activeTab.element).marginRight;
        const tabMarginRight = pixelsToNumber(tabMarginRightPixels);
        const tabWidth = tabElement.offsetWidth + tabMarginRight;
        cumulativeTabWidth += tabWidth;
        let visibleTabWidth = 0;
        if (activeIndex <= i) {
          visibleTabWidth = cumulativeTabWidth;
        } else {
          const activeTabMarginRightPixels = getComputedStyle(activeTab.element).marginRight;
          const activeTabMarginRight = pixelsToNumber(activeTabMarginRightPixels);
          visibleTabWidth = cumulativeTabWidth + activeTab.element.offsetWidth + activeTabMarginRight;
        }
        if (visibleTabWidth > availableWidth) {
          if (!tabOverlapAllowanceExceeded) {
            let overlap;
            if (activeIndex > 0 && activeIndex <= i) {
              overlap = (visibleTabWidth - availableWidth) / (i - 1);
            } else {
              overlap = (visibleTabWidth - availableWidth) / i;
            }
            if (overlap < tabOverlapAllowance) {
              for (let j = 0; j <= i; j++) {
                const marginLeft = j !== activeIndex && j !== 0 ? "-" + numberToPixels(overlap) : "";
                this._tabs[j].element.style.zIndex = numberToPixels(i - j);
                this._tabs[j].element.style.marginLeft = marginLeft;
              }
              this._lastVisibleTabIndex = i;
              if (tabElement.parentElement !== this._element) {
                this._element.appendChild(tabElement);
              }
            } else {
              tabOverlapAllowanceExceeded = true;
            }
          } else if (i === activeIndex) {
            tabElement.style.zIndex = "auto";
            tabElement.style.marginLeft = "";
            if (tabElement.parentElement !== this._element) {
              this._element.appendChild(tabElement);
            }
          }
          if (tabOverlapAllowanceExceeded && i !== activeIndex) {
            if (dropdownActive) {
              tabElement.style.zIndex = "auto";
              tabElement.style.marginLeft = "";
              if (tabElement.parentElement !== this._dropdownElement) {
                this._dropdownElement.appendChild(tabElement);
              }
            } else {
              return false;
            }
          }
        } else {
          this._lastVisibleTabIndex = i;
          tabElement.style.zIndex = "auto";
          tabElement.style.marginLeft = "";
          if (tabElement.parentElement !== this._element) {
            this._element.appendChild(tabElement);
          }
        }
      }
    }
    return true;
  }
  /**
   * Shows drop down for additional tabs when there are too many to display.
   */
  showAdditionalTabsDropdown() {
    this._dropdownElement.style.display = "";
  }
  /**
   * Hides drop down for additional tabs when there are too many to display.
   */
  hideAdditionalTabsDropdown() {
    this._dropdownElement.style.display = "none";
  }
  handleTabCloseEvent(componentItem) {
    this._componentRemoveEvent(componentItem);
  }
  handleTabFocusEvent(componentItem) {
    this._componentFocusEvent(componentItem);
  }
  handleTabDragStartEvent(x, y, dragListener, componentItem) {
    this._componentDragStartEvent(x, y, dragListener, componentItem);
  }
};

// dist/esm/ts/controls/header.js
var Header = class extends EventEmitter {
  /** @internal */
  constructor(_layoutManager, _parent, settings, _configClosable, _getActiveComponentItemEvent, closeEvent, _popoutEvent, _maximiseToggleEvent, _clickEvent, _touchStartEvent, _componentRemoveEvent, _componentFocusEvent, _componentDragStartEvent) {
    super();
    this._layoutManager = _layoutManager;
    this._parent = _parent;
    this._configClosable = _configClosable;
    this._getActiveComponentItemEvent = _getActiveComponentItemEvent;
    this._popoutEvent = _popoutEvent;
    this._maximiseToggleEvent = _maximiseToggleEvent;
    this._clickEvent = _clickEvent;
    this._touchStartEvent = _touchStartEvent;
    this._componentRemoveEvent = _componentRemoveEvent;
    this._componentFocusEvent = _componentFocusEvent;
    this._componentDragStartEvent = _componentDragStartEvent;
    this._clickListener = (ev) => this.onClick(ev);
    this._touchStartListener = (ev) => this.onTouchStart(ev);
    this._rowColumnClosable = true;
    this._closeButton = null;
    this._popoutButton = null;
    this._tabsContainer = new TabsContainer(this._layoutManager, (item) => this.handleTabInitiatedComponentRemoveEvent(item), (item) => this.handleTabInitiatedComponentFocusEvent(item), (x, y, dragListener, item) => this.handleTabInitiatedDragStartEvent(x, y, dragListener, item), () => this.processTabDropdownActiveChanged());
    this._show = settings.show;
    this._popoutEnabled = settings.popoutEnabled;
    this._popoutLabel = settings.popoutLabel;
    this._maximiseEnabled = settings.maximiseEnabled;
    this._maximiseLabel = settings.maximiseLabel;
    this._minimiseEnabled = settings.minimiseEnabled;
    this._minimiseLabel = settings.minimiseLabel;
    this._closeEnabled = settings.closeEnabled;
    this._closeLabel = settings.closeLabel;
    this._tabDropdownEnabled = settings.tabDropdownEnabled;
    this._tabDropdownLabel = settings.tabDropdownLabel;
    this.setSide(settings.side);
    this._canRemoveComponent = this._configClosable;
    this._element = document.createElement("section");
    this._element.classList.add(
      "lm_header"
      /* Header */
    );
    this._controlsContainerElement = document.createElement("section");
    this._controlsContainerElement.classList.add(
      "lm_controls"
      /* Controls */
    );
    this._element.appendChild(this._tabsContainer.element);
    this._element.appendChild(this._controlsContainerElement);
    this._element.appendChild(this._tabsContainer.dropdownElement);
    this._element.addEventListener("click", this._clickListener, { passive: true });
    this._element.addEventListener("touchstart", this._touchStartListener, { passive: true });
    this._documentMouseUpListener = () => this._tabsContainer.hideAdditionalTabsDropdown();
    globalThis.document.addEventListener("mouseup", this._documentMouseUpListener, { passive: true });
    this._tabControlOffset = this._layoutManager.layoutConfig.settings.tabControlOffset;
    if (this._tabDropdownEnabled) {
      this._tabDropdownButton = new HeaderButton(this, this._tabDropdownLabel, "lm_tabdropdown", () => this._tabsContainer.showAdditionalTabsDropdown());
    }
    if (this._popoutEnabled) {
      this._popoutButton = new HeaderButton(this, this._popoutLabel, "lm_popout", () => this.handleButtonPopoutEvent());
    }
    if (this._maximiseEnabled) {
      this._maximiseButton = new HeaderButton(this, this._maximiseLabel, "lm_maximise", (ev) => this.handleButtonMaximiseToggleEvent(ev));
    }
    if (this._configClosable) {
      this._closeButton = new HeaderButton(this, this._closeLabel, "lm_close", () => closeEvent());
    }
    this.processTabDropdownActiveChanged();
  }
  // /** @internal */
  // private _activeComponentItem: ComponentItem | null = null; // only used to identify active tab
  get show() {
    return this._show;
  }
  get side() {
    return this._side;
  }
  get leftRightSided() {
    return this._leftRightSided;
  }
  get layoutManager() {
    return this._layoutManager;
  }
  get parent() {
    return this._parent;
  }
  get tabs() {
    return this._tabsContainer.tabs;
  }
  get lastVisibleTabIndex() {
    return this._tabsContainer.lastVisibleTabIndex;
  }
  get element() {
    return this._element;
  }
  get tabsContainerElement() {
    return this._tabsContainer.element;
  }
  get controlsContainerElement() {
    return this._controlsContainerElement;
  }
  /**
   * Destroys the entire header
   * @internal
   */
  destroy() {
    this.emit("destroy");
    this._popoutEvent = void 0;
    this._maximiseToggleEvent = void 0;
    this._clickEvent = void 0;
    this._touchStartEvent = void 0;
    this._componentRemoveEvent = void 0;
    this._componentFocusEvent = void 0;
    this._componentDragStartEvent = void 0;
    this._tabsContainer.destroy();
    globalThis.document.removeEventListener("mouseup", this._documentMouseUpListener);
    this._element.remove();
  }
  /**
   * Creates a new tab and associates it with a contentItem
   * @param index - The position of the tab
   * @internal
   */
  createTab(componentItem, index) {
    this._tabsContainer.createTab(componentItem, index);
  }
  /**
   * Finds a tab based on the contentItem its associated with and removes it.
   * Cannot remove tab if it has the active ComponentItem
   * @internal
   */
  removeTab(componentItem) {
    this._tabsContainer.removeTab(componentItem);
  }
  /** @internal */
  processActiveComponentChanged(newActiveComponentItem) {
    this._tabsContainer.processActiveComponentChanged(newActiveComponentItem);
    this.updateTabSizes();
  }
  /** @internal */
  setSide(value) {
    this._side = value;
    this._leftRightSided = [Side.right, Side.left].includes(this._side);
  }
  /**
   * Programmatically set closability.
   * @param value - Whether to enable/disable closability.
   * @returns Whether the action was successful
   * @internal
   */
  setRowColumnClosable(value) {
    this._rowColumnClosable = value;
    this.updateClosability();
  }
  /**
   * Updates the header's closability. If a stack/header is able
   * to close, but has a non closable component added to it, the stack is no
   * longer closable until all components are closable.
   * @internal
   */
  updateClosability() {
    let isClosable;
    if (!this._configClosable) {
      isClosable = false;
    } else {
      if (!this._rowColumnClosable) {
        isClosable = false;
      } else {
        isClosable = true;
        const len = this.tabs.length;
        for (let i = 0; i < len; i++) {
          const tab = this._tabsContainer.tabs[i];
          const item = tab.componentItem;
          if (!item.isClosable) {
            isClosable = false;
            break;
          }
        }
      }
    }
    if (this._closeButton !== null) {
      setElementDisplayVisibility(this._closeButton.element, isClosable);
    }
    if (this._popoutButton !== null) {
      setElementDisplayVisibility(this._popoutButton.element, isClosable);
    }
    this._canRemoveComponent = isClosable || this._tabsContainer.tabCount > 1;
  }
  /** @internal */
  applyFocusedValue(value) {
    if (value) {
      this._element.classList.add(
        "lm_focused"
        /* Focused */
      );
    } else {
      this._element.classList.remove(
        "lm_focused"
        /* Focused */
      );
    }
  }
  /** @internal */
  processMaximised() {
    if (this._maximiseButton === void 0) {
      throw new UnexpectedUndefinedError("HPMAX16997");
    } else {
      this._maximiseButton.element.setAttribute("title", this._minimiseLabel);
    }
  }
  /** @internal */
  processMinimised() {
    if (this._maximiseButton === void 0) {
      throw new UnexpectedUndefinedError("HPMIN16997");
    } else {
      this._maximiseButton.element.setAttribute("title", this._maximiseLabel);
    }
  }
  /**
   * Pushes the tabs to the tab dropdown if the available space is not sufficient
   * @internal
   */
  updateTabSizes() {
    if (this._tabsContainer.tabCount > 0) {
      const headerHeight = this._show ? this._layoutManager.layoutConfig.dimensions.headerHeight : 0;
      if (this._leftRightSided) {
        this._element.style.height = "";
        this._element.style.width = numberToPixels(headerHeight);
      } else {
        this._element.style.width = "";
        this._element.style.height = numberToPixels(headerHeight);
      }
      let availableWidth;
      if (this._leftRightSided) {
        availableWidth = this._element.offsetHeight - this._controlsContainerElement.offsetHeight - this._tabControlOffset;
      } else {
        availableWidth = this._element.offsetWidth - this._controlsContainerElement.offsetWidth - this._tabControlOffset;
      }
      this._tabsContainer.updateTabSizes(availableWidth, this._getActiveComponentItemEvent());
    }
  }
  /** @internal */
  handleTabInitiatedComponentRemoveEvent(componentItem) {
    if (this._canRemoveComponent) {
      if (this._componentRemoveEvent === void 0) {
        throw new UnexpectedUndefinedError("HHTCE22294");
      } else {
        this._componentRemoveEvent(componentItem);
      }
    }
  }
  /** @internal */
  handleTabInitiatedComponentFocusEvent(componentItem) {
    if (this._componentFocusEvent === void 0) {
      throw new UnexpectedUndefinedError("HHTAE22294");
    } else {
      this._componentFocusEvent(componentItem);
    }
  }
  /** @internal */
  handleTabInitiatedDragStartEvent(x, y, dragListener, componentItem) {
    if (!this._canRemoveComponent) {
      dragListener.cancelDrag();
    } else {
      if (this._componentDragStartEvent === void 0) {
        throw new UnexpectedUndefinedError("HHTDSE22294");
      } else {
        this._componentDragStartEvent(x, y, dragListener, componentItem);
      }
    }
  }
  /** @internal */
  processTabDropdownActiveChanged() {
    if (this._tabDropdownButton !== void 0) {
      setElementDisplayVisibility(this._tabDropdownButton.element, this._tabsContainer.dropdownActive);
    }
  }
  /** @internal */
  handleButtonPopoutEvent() {
    if (this._layoutManager.layoutConfig.settings.popoutWholeStack) {
      if (this._popoutEvent === void 0) {
        throw new UnexpectedUndefinedError("HHBPOE17834");
      } else {
        this._popoutEvent();
      }
    } else {
      const activeComponentItem = this._getActiveComponentItemEvent();
      if (activeComponentItem) {
        activeComponentItem.popout();
      }
    }
  }
  /** @internal */
  // eslint-disable-next-line @typescript-eslint/no-unused-vars
  handleButtonMaximiseToggleEvent(ev) {
    if (this._maximiseToggleEvent === void 0) {
      throw new UnexpectedUndefinedError("HHBMTE16834");
    } else {
      this._maximiseToggleEvent();
    }
  }
  /**
   * Invoked when the header's background is clicked (not it's tabs or controls)
   * @internal
   */
  onClick(event) {
    if (event.target === this._element) {
      this.notifyClick(event);
    }
  }
  /**
   * Invoked when the header's background is touched (not it's tabs or controls)
   * @internal
   */
  onTouchStart(event) {
    if (event.target === this._element) {
      this.notifyTouchStart(event);
    }
  }
  /** @internal */
  notifyClick(ev) {
    if (this._clickEvent === void 0) {
      throw new UnexpectedUndefinedError("HNHC46834");
    } else {
      this._clickEvent(ev);
    }
  }
  /** @internal */
  notifyTouchStart(ev) {
    if (this._touchStartEvent === void 0) {
      throw new UnexpectedUndefinedError("HNHTS46834");
    } else {
      this._touchStartEvent(ev);
    }
  }
};

// dist/esm/ts/items/stack.js
var Stack = class _Stack extends ComponentParentableItem {
  /** @internal */
  constructor(layoutManager, config, parent) {
    var _a, _b, _c, _d, _e, _f, _g, _h, _j, _k, _l, _m, _o, _p, _q, _r, _s, _t, _u;
    super(layoutManager, config, parent, _Stack.createElement(document));
    this._headerSideChanged = false;
    this._resizeListener = () => this.handleResize();
    this._maximisedListener = () => this.handleMaximised();
    this._minimisedListener = () => this.handleMinimised();
    this._headerConfig = config.header;
    const layoutHeaderConfig = layoutManager.layoutConfig.header;
    const configContent = config.content;
    let componentHeaderConfig;
    if (configContent.length !== 1) {
      componentHeaderConfig = void 0;
    } else {
      const firstChildItemConfig = configContent[0];
      componentHeaderConfig = firstChildItemConfig.header;
    }
    this._initialWantMaximise = config.maximised;
    this._initialActiveItemIndex = (_a = config.activeItemIndex) !== null && _a !== void 0 ? _a : 0;
    const show = (_d = (_c = (_b = this._headerConfig) === null || _b === void 0 ? void 0 : _b.show) !== null && _c !== void 0 ? _c : componentHeaderConfig === null || componentHeaderConfig === void 0 ? void 0 : componentHeaderConfig.show) !== null && _d !== void 0 ? _d : layoutHeaderConfig.show;
    const popout = (_g = (_f = (_e = this._headerConfig) === null || _e === void 0 ? void 0 : _e.popout) !== null && _f !== void 0 ? _f : componentHeaderConfig === null || componentHeaderConfig === void 0 ? void 0 : componentHeaderConfig.popout) !== null && _g !== void 0 ? _g : layoutHeaderConfig.popout;
    const maximise = (_k = (_j = (_h = this._headerConfig) === null || _h === void 0 ? void 0 : _h.maximise) !== null && _j !== void 0 ? _j : componentHeaderConfig === null || componentHeaderConfig === void 0 ? void 0 : componentHeaderConfig.maximise) !== null && _k !== void 0 ? _k : layoutHeaderConfig.maximise;
    const close = (_o = (_m = (_l = this._headerConfig) === null || _l === void 0 ? void 0 : _l.close) !== null && _m !== void 0 ? _m : componentHeaderConfig === null || componentHeaderConfig === void 0 ? void 0 : componentHeaderConfig.close) !== null && _o !== void 0 ? _o : layoutHeaderConfig.close;
    const minimise = (_r = (_q = (_p = this._headerConfig) === null || _p === void 0 ? void 0 : _p.minimise) !== null && _q !== void 0 ? _q : componentHeaderConfig === null || componentHeaderConfig === void 0 ? void 0 : componentHeaderConfig.minimise) !== null && _r !== void 0 ? _r : layoutHeaderConfig.minimise;
    const tabDropdown = (_u = (_t = (_s = this._headerConfig) === null || _s === void 0 ? void 0 : _s.tabDropdown) !== null && _t !== void 0 ? _t : componentHeaderConfig === null || componentHeaderConfig === void 0 ? void 0 : componentHeaderConfig.tabDropdown) !== null && _u !== void 0 ? _u : layoutHeaderConfig.tabDropdown;
    this._maximisedEnabled = maximise !== false;
    const headerSettings = {
      show: show !== false,
      side: show === false ? Side.top : show,
      popoutEnabled: popout !== false,
      popoutLabel: popout === false ? "" : popout,
      maximiseEnabled: this._maximisedEnabled,
      maximiseLabel: maximise === false ? "" : maximise,
      closeEnabled: close !== false,
      closeLabel: close === false ? "" : close,
      minimiseEnabled: true,
      minimiseLabel: minimise,
      tabDropdownEnabled: tabDropdown !== false,
      tabDropdownLabel: tabDropdown === false ? "" : tabDropdown
    };
    this._header = new Header(layoutManager, this, headerSettings, config.isClosable && close !== false, () => this.getActiveComponentItem(), () => this.remove(), () => this.handlePopoutEvent(), () => this.toggleMaximise(), (ev) => this.handleHeaderClickEvent(ev), (ev) => this.handleHeaderTouchStartEvent(ev), (item) => this.handleHeaderComponentRemoveEvent(item), (item) => this.handleHeaderComponentFocusEvent(item), (x, y, dragListener, item) => this.handleHeaderComponentStartDragEvent(x, y, dragListener, item));
    this.isStack = true;
    this._childElementContainer = document.createElement("section");
    this._childElementContainer.classList.add(
      "lm_items"
      /* Items */
    );
    this.on("resize", this._resizeListener);
    if (this._maximisedEnabled) {
      this.on("maximised", this._maximisedListener);
      this.on("minimised", this._minimisedListener);
    }
    this.element.appendChild(this._header.element);
    this.element.appendChild(this._childElementContainer);
    this.setupHeaderPosition();
    this._header.updateClosability();
  }
  get childElementContainer() {
    return this._childElementContainer;
  }
  get header() {
    return this._header;
  }
  get headerShow() {
    return this._header.show;
  }
  get headerSide() {
    return this._header.side;
  }
  get headerLeftRightSided() {
    return this._header.leftRightSided;
  }
  /** @internal */
  get contentAreaDimensions() {
    return this._contentAreaDimensions;
  }
  /** @internal */
  get initialWantMaximise() {
    return this._initialWantMaximise;
  }
  get isMaximised() {
    return this === this.layoutManager.maximisedStack;
  }
  get stackParent() {
    if (!this.parent) {
      throw new Error("Stack should always have a parent");
    }
    return this.parent;
  }
  /** @internal */
  updateSize(force) {
    this.layoutManager.beginVirtualSizedContainerAdding();
    try {
      this.updateNodeSize();
      this.updateContentItemsSize(force);
    } finally {
      this.layoutManager.endVirtualSizedContainerAdding();
    }
  }
  /** @internal */
  init() {
    if (this.isInitialised === true)
      return;
    this.updateNodeSize();
    for (let i = 0; i < this.contentItems.length; i++) {
      this._childElementContainer.appendChild(this.contentItems[i].element);
    }
    super.init();
    const contentItems = this.contentItems;
    const contentItemCount = contentItems.length;
    if (contentItemCount > 0) {
      if (this._initialActiveItemIndex < 0 || this._initialActiveItemIndex >= contentItemCount) {
        throw new Error(`ActiveItemIndex out of range: ${this._initialActiveItemIndex} id: ${this.id}`);
      } else {
        for (let i = 0; i < contentItemCount; i++) {
          const contentItem = contentItems[i];
          if (!(contentItem instanceof ComponentItem)) {
            throw new Error(`Stack Content Item is not of type ComponentItem: ${i} id: ${this.id}`);
          } else {
            this._header.createTab(contentItem, i);
            contentItem.hide();
            contentItem.container.setBaseLogicalZIndex();
          }
        }
        this.setActiveComponentItem(contentItems[this._initialActiveItemIndex], false);
        this._header.updateTabSizes();
      }
    }
    this._header.updateClosability();
    this.initContentItems();
  }
  /** @deprecated Use {@link (Stack:class).setActiveComponentItem} */
  setActiveContentItem(item) {
    if (!ContentItem.isComponentItem(item)) {
      throw new Error("Stack.setActiveContentItem: item is not a ComponentItem");
    } else {
      this.setActiveComponentItem(item, false);
    }
  }
  setActiveComponentItem(componentItem, focus, suppressFocusEvent = false) {
    if (this._activeComponentItem !== componentItem) {
      if (this.contentItems.indexOf(componentItem) === -1) {
        throw new Error("componentItem is not a child of this stack");
      } else {
        this.layoutManager.beginSizeInvalidation();
        try {
          if (this._activeComponentItem !== void 0) {
            this._activeComponentItem.hide();
          }
          this._activeComponentItem = componentItem;
          this._header.processActiveComponentChanged(componentItem);
          componentItem.show();
        } finally {
          this.layoutManager.endSizeInvalidation();
        }
        this.emit("activeContentItemChanged", componentItem);
        this.layoutManager.emit("activeContentItemChanged", componentItem);
        this.emitStateChangedEvent();
      }
    }
    if (this.focused || focus) {
      this.layoutManager.setFocusedComponentItem(componentItem, suppressFocusEvent);
    }
  }
  /** @deprecated Use {@link (Stack:class).getActiveComponentItem} */
  getActiveContentItem() {
    var _a;
    return (_a = this.getActiveComponentItem()) !== null && _a !== void 0 ? _a : null;
  }
  getActiveComponentItem() {
    return this._activeComponentItem;
  }
  /** @internal */
  focusActiveContentItem() {
    var _a;
    (_a = this._activeComponentItem) === null || _a === void 0 ? void 0 : _a.focus();
  }
  /** @internal */
  setFocusedValue(value) {
    this._header.applyFocusedValue(value);
    super.setFocusedValue(value);
  }
  /** @internal */
  setRowColumnClosable(value) {
    this._header.setRowColumnClosable(value);
  }
  newComponent(componentType, componentState, title, index) {
    const itemConfig = {
      type: "component",
      componentType,
      componentState,
      title
    };
    return this.newItem(itemConfig, index);
  }
  addComponent(componentType, componentState, title, index) {
    const itemConfig = {
      type: "component",
      componentType,
      componentState,
      title
    };
    return this.addItem(itemConfig, index);
  }
  newItem(itemConfig, index) {
    index = this.addItem(itemConfig, index);
    return this.contentItems[index];
  }
  addItem(itemConfig, index) {
    this.layoutManager.checkMinimiseMaximisedStack();
    const resolvedItemConfig = ItemConfig.resolve(itemConfig, false);
    const contentItem = this.layoutManager.createAndInitContentItem(resolvedItemConfig, this);
    return this.addChild(contentItem, index);
  }
  addChild(contentItem, index, focus = false) {
    if (index !== void 0 && index > this.contentItems.length) {
      index -= 1;
      throw new AssertError("SAC99728");
    }
    if (!(contentItem instanceof ComponentItem)) {
      throw new AssertError("SACC88532");
    } else {
      index = super.addChild(contentItem, index);
      this._childElementContainer.appendChild(contentItem.element);
      this._header.createTab(contentItem, index);
      this.setActiveComponentItem(contentItem, focus);
      this._header.updateTabSizes();
      this.updateSize(false);
      contentItem.container.setBaseLogicalZIndex();
      this._header.updateClosability();
      this.emitStateChangedEvent();
      return index;
    }
  }
  removeChild(contentItem, keepChild) {
    const componentItem = contentItem;
    const index = this.contentItems.indexOf(componentItem);
    const stackWillBeDeleted = this.contentItems.length === 1;
    if (this._activeComponentItem === componentItem) {
      if (componentItem.focused) {
        componentItem.blur();
      }
      if (!stackWillBeDeleted) {
        const newActiveComponentIdx = index === 0 ? 1 : index - 1;
        this.setActiveComponentItem(this.contentItems[newActiveComponentIdx], false);
      }
    }
    this._header.removeTab(componentItem);
    super.removeChild(componentItem, keepChild);
    if (!stackWillBeDeleted) {
      this._header.updateClosability();
    }
    this.emitStateChangedEvent();
  }
  /**
   * Maximises the Item or minimises it if it is already maximised
   */
  toggleMaximise() {
    if (this.isMaximised) {
      this.minimise();
    } else {
      this.maximise();
    }
  }
  maximise() {
    if (!this.isMaximised) {
      this.layoutManager.setMaximisedStack(this);
      const contentItems = this.contentItems;
      const contentItemCount = contentItems.length;
      for (let i = 0; i < contentItemCount; i++) {
        const contentItem = contentItems[i];
        if (contentItem instanceof ComponentItem) {
          contentItem.enterStackMaximised();
        } else {
          throw new AssertError("SMAXI87773");
        }
      }
      this.emitStateChangedEvent();
    }
  }
  minimise() {
    if (this.isMaximised) {
      this.layoutManager.setMaximisedStack(void 0);
      const contentItems = this.contentItems;
      const contentItemCount = contentItems.length;
      for (let i = 0; i < contentItemCount; i++) {
        const contentItem = contentItems[i];
        if (contentItem instanceof ComponentItem) {
          contentItem.exitStackMaximised();
        } else {
          throw new AssertError("SMINI87773");
        }
      }
      this.emitStateChangedEvent();
    }
  }
  /** @internal */
  destroy() {
    var _a;
    if ((_a = this._activeComponentItem) === null || _a === void 0 ? void 0 : _a.focused) {
      this._activeComponentItem.blur();
    }
    super.destroy();
    this.off("resize", this._resizeListener);
    if (this._maximisedEnabled) {
      this.off("maximised", this._maximisedListener);
      this.off("minimised", this._minimisedListener);
    }
    this._header.destroy();
  }
  toConfig() {
    let activeItemIndex;
    if (this._activeComponentItem) {
      activeItemIndex = this.contentItems.indexOf(this._activeComponentItem);
      if (activeItemIndex < 0) {
        throw new Error("active component item not found in stack");
      }
    }
    if (this.contentItems.length > 0 && activeItemIndex === void 0) {
      throw new Error("expected non-empty stack to have an active component item");
    } else {
      const result = {
        type: "stack",
        content: this.calculateConfigContent(),
        size: this.size,
        sizeUnit: this.sizeUnit,
        minSize: this.minSize,
        minSizeUnit: this.minSizeUnit,
        id: this.id,
        isClosable: this.isClosable,
        maximised: this.isMaximised,
        header: this.createHeaderConfig(),
        activeItemIndex
      };
      return result;
    }
  }
  /**
   * Ok, this one is going to be the tricky one: The user has dropped a {@link (ContentItem:class)} onto this stack.
   *
   * It was dropped on either the stacks header or the top, right, bottom or left bit of the content area
   * (which one of those is stored in this._dropSegment). Now, if the user has dropped on the header the case
   * is relatively clear: We add the item to the existing stack... job done (might be good to have
   * tab reordering at some point, but lets not sweat it right now)
   *
   * If the item was dropped on the content part things are a bit more complicated. If it was dropped on either the
   * top or bottom region we need to create a new column and place the items accordingly.
   * Unless, of course if the stack is already within a column... in which case we want
   * to add the newly created item to the existing column...
   * either prepend or append it, depending on wether its top or bottom.
   *
   * Same thing for rows and left / right drop segments... so in total there are 9 things that can potentially happen
   * (left, top, right, bottom) * is child of the right parent (row, column) + header drop
   *
   * @internal
   */
  // eslint-disable-next-line @typescript-eslint/no-unused-vars
  onDrop(contentItem, area) {
    if (this._dropSegment === "header") {
      this.resetHeaderDropZone();
      if (this._dropIndex === void 0) {
        throw new UnexpectedUndefinedError("SODDI68990");
      } else {
        this.addChild(contentItem, this._dropIndex);
        return;
      }
    }
    if (this._dropSegment === "body") {
      this.addChild(contentItem, 0, true);
      return;
    }
    const isVertical = this._dropSegment === "top" || this._dropSegment === "bottom";
    const isHorizontal = this._dropSegment === "left" || this._dropSegment === "right";
    const insertBefore = this._dropSegment === "top" || this._dropSegment === "left";
    const hasCorrectParent = isVertical && this.stackParent.isColumn || isHorizontal && this.stackParent.isRow;
    if (contentItem.isComponent) {
      const itemConfig = ResolvedStackItemConfig.createDefault();
      itemConfig.header = this.createHeaderConfig();
      const stack = this.layoutManager.createAndInitContentItem(itemConfig, this);
      stack.addChild(contentItem);
      contentItem = stack;
    }
    if (contentItem.type === ItemType.row || contentItem.type === ItemType.column) {
      const itemConfig = ResolvedStackItemConfig.createDefault();
      itemConfig.header = this.createHeaderConfig();
      const stack = this.layoutManager.createContentItem(itemConfig, this);
      stack.addChild(contentItem);
      contentItem = stack;
    }
    if (hasCorrectParent) {
      const index = this.stackParent.contentItems.indexOf(this);
      this.stackParent.addChild(contentItem, insertBefore ? index : index + 1, true);
      this.size *= 0.5;
      contentItem.size = this.size;
      contentItem.sizeUnit = this.sizeUnit;
      this.stackParent.updateSize(false);
    } else {
      const type = isVertical ? ItemType.column : ItemType.row;
      const itemConfig = ResolvedItemConfig.createDefault(type);
      const rowOrColumn = this.layoutManager.createContentItem(itemConfig, this);
      this.stackParent.replaceChild(this, rowOrColumn);
      rowOrColumn.addChild(contentItem, insertBefore ? 0 : void 0, true);
      rowOrColumn.addChild(this, insertBefore ? void 0 : 0, true);
      this.size = 50;
      contentItem.size = 50;
      contentItem.sizeUnit = SizeUnitEnum.Percent;
      rowOrColumn.updateSize(false);
    }
  }
  /**
   * If the user hovers above the header part of the stack, indicate drop positions for tabs.
   * otherwise indicate which segment of the body the dragged item would be dropped on
   *
   * @param x - Absolute Screen X
   * @param y - Absolute Screen Y
   * @internal
   */
  highlightDropZone(x, y) {
    for (const key in this._contentAreaDimensions) {
      const segment = key;
      const area = this._contentAreaDimensions[segment].hoverArea;
      if (area.x1 < x && area.x2 > x && area.y1 < y && area.y2 > y) {
        if (segment === "header") {
          this._dropSegment = "header";
          this.highlightHeaderDropZone(this._header.leftRightSided ? y : x);
        } else {
          this.resetHeaderDropZone();
          this.highlightBodyDropZone(segment);
        }
        return;
      }
    }
  }
  /** @internal */
  getArea() {
    if (this.element.style.display === "none") {
      return null;
    }
    const headerArea = super.getElementArea(this._header.element);
    const contentArea = super.getElementArea(this._childElementContainer);
    if (headerArea === null || contentArea === null) {
      throw new UnexpectedNullError("SGAHC13086");
    }
    const contentWidth = contentArea.x2 - contentArea.x1;
    const contentHeight = contentArea.y2 - contentArea.y1;
    this._contentAreaDimensions = {
      header: {
        hoverArea: {
          x1: headerArea.x1,
          y1: headerArea.y1,
          x2: headerArea.x2,
          y2: headerArea.y2
        },
        highlightArea: {
          x1: headerArea.x1,
          y1: headerArea.y1,
          x2: headerArea.x2,
          y2: headerArea.y2
        }
      }
    };
    if (this.contentItems.length === 0) {
      this._contentAreaDimensions.body = {
        hoverArea: {
          x1: contentArea.x1,
          y1: contentArea.y1,
          x2: contentArea.x2,
          y2: contentArea.y2
        },
        highlightArea: {
          x1: contentArea.x1,
          y1: contentArea.y1,
          x2: contentArea.x2,
          y2: contentArea.y2
        }
      };
      return super.getElementArea(this.element);
    } else {
      this._contentAreaDimensions.left = {
        hoverArea: {
          x1: contentArea.x1,
          y1: contentArea.y1,
          x2: contentArea.x1 + contentWidth * 0.25,
          y2: contentArea.y2
        },
        highlightArea: {
          x1: contentArea.x1,
          y1: contentArea.y1,
          x2: contentArea.x1 + contentWidth * 0.5,
          y2: contentArea.y2
        }
      };
      this._contentAreaDimensions.top = {
        hoverArea: {
          x1: contentArea.x1 + contentWidth * 0.25,
          y1: contentArea.y1,
          x2: contentArea.x1 + contentWidth * 0.75,
          y2: contentArea.y1 + contentHeight * 0.5
        },
        highlightArea: {
          x1: contentArea.x1,
          y1: contentArea.y1,
          x2: contentArea.x2,
          y2: contentArea.y1 + contentHeight * 0.5
        }
      };
      this._contentAreaDimensions.right = {
        hoverArea: {
          x1: contentArea.x1 + contentWidth * 0.75,
          y1: contentArea.y1,
          x2: contentArea.x2,
          y2: contentArea.y2
        },
        highlightArea: {
          x1: contentArea.x1 + contentWidth * 0.5,
          y1: contentArea.y1,
          x2: contentArea.x2,
          y2: contentArea.y2
        }
      };
      this._contentAreaDimensions.bottom = {
        hoverArea: {
          x1: contentArea.x1 + contentWidth * 0.25,
          y1: contentArea.y1 + contentHeight * 0.5,
          x2: contentArea.x1 + contentWidth * 0.75,
          y2: contentArea.y2
        },
        highlightArea: {
          x1: contentArea.x1,
          y1: contentArea.y1 + contentHeight * 0.5,
          x2: contentArea.x2,
          y2: contentArea.y2
        }
      };
      return super.getElementArea(this.element);
    }
  }
  /**
   * Programmatically operate with header position.
   *
   * @param position -
   *
   * @returns previous header position
   * @internal
   */
  positionHeader(position) {
    if (this._header.side !== position) {
      this._header.setSide(position);
      this._headerSideChanged = true;
      this.setupHeaderPosition();
    }
  }
  /** @internal */
  updateNodeSize() {
    if (this.element.style.display !== "none") {
      const content = getElementWidthAndHeight(this.element);
      if (this._header.show) {
        const dimension = this._header.leftRightSided ? WidthOrHeightPropertyName.width : WidthOrHeightPropertyName.height;
        content[dimension] -= this.layoutManager.layoutConfig.dimensions.headerHeight;
      }
      this._childElementContainer.style.width = numberToPixels(content.width);
      this._childElementContainer.style.height = numberToPixels(content.height);
      for (let i = 0; i < this.contentItems.length; i++) {
        this.contentItems[i].element.style.width = numberToPixels(content.width);
        this.contentItems[i].element.style.height = numberToPixels(content.height);
      }
      this.emit("resize");
      this.emitStateChangedEvent();
    }
  }
  /** @internal */
  highlightHeaderDropZone(x) {
    const visibleTabsLength = this._header.lastVisibleTabIndex + 1;
    const tabsContainerElement = this._header.tabsContainerElement;
    const tabsContainerElementChildNodes = tabsContainerElement.childNodes;
    const visibleTabElements = new Array(visibleTabsLength);
    let tabIndex = 0;
    let tabCount = 0;
    while (tabCount < visibleTabsLength) {
      const visibleTabElement = tabsContainerElementChildNodes[tabIndex++];
      if (visibleTabElement !== this.layoutManager.tabDropPlaceholder) {
        visibleTabElements[tabCount++] = visibleTabElement;
      }
    }
    const dropTargetIndicator = this.layoutManager.dropTargetIndicator;
    if (dropTargetIndicator === null) {
      throw new UnexpectedNullError("SHHDZDTI97110");
    }
    let area;
    if (visibleTabsLength === 0) {
      const headerRect = this._header.element.getBoundingClientRect();
      const headerTop = headerRect.top + document.body.scrollTop;
      const headerLeft = headerRect.left + document.body.scrollLeft;
      area = {
        x1: headerLeft,
        x2: headerLeft + 100,
        y1: headerTop + headerRect.height - 20,
        y2: headerTop + headerRect.height
      };
      this._dropIndex = 0;
    } else {
      let tabIndex2 = 0;
      let isAboveTab = false;
      let tabTop;
      let tabLeft;
      let tabWidth;
      let tabElement;
      do {
        tabElement = visibleTabElements[tabIndex2];
        const tabRect = tabElement.getBoundingClientRect();
        const tabRectTop = tabRect.top + document.body.scrollTop;
        const tabRectLeft = tabRect.left + document.body.scrollLeft;
        if (this._header.leftRightSided) {
          tabLeft = tabRectTop;
          tabTop = tabRectLeft;
          tabWidth = tabRect.height;
        } else {
          tabLeft = tabRectLeft;
          tabTop = tabRectTop;
          tabWidth = tabRect.width;
        }
        if (x >= tabLeft && x < tabLeft + tabWidth) {
          isAboveTab = true;
        } else {
          tabIndex2++;
        }
      } while (tabIndex2 < visibleTabsLength && !isAboveTab);
      if (isAboveTab === false && x < tabLeft) {
        return;
      }
      const halfX = tabLeft + tabWidth / 2;
      if (x < halfX) {
        this._dropIndex = tabIndex2;
        tabElement.insertAdjacentElement("beforebegin", this.layoutManager.tabDropPlaceholder);
      } else {
        this._dropIndex = Math.min(tabIndex2 + 1, visibleTabsLength);
        tabElement.insertAdjacentElement("afterend", this.layoutManager.tabDropPlaceholder);
      }
      const tabDropPlaceholderRect = this.layoutManager.tabDropPlaceholder.getBoundingClientRect();
      const tabDropPlaceholderRectTop = tabDropPlaceholderRect.top + document.body.scrollTop;
      const tabDropPlaceholderRectLeft = tabDropPlaceholderRect.left + document.body.scrollLeft;
      const tabDropPlaceholderRectWidth = tabDropPlaceholderRect.width;
      if (this._header.leftRightSided) {
        const placeHolderTop = tabDropPlaceholderRectTop;
        area = {
          x1: tabTop,
          x2: tabTop + tabElement.clientHeight,
          y1: placeHolderTop,
          y2: placeHolderTop + tabDropPlaceholderRectWidth
        };
      } else {
        const placeHolderLeft = tabDropPlaceholderRectLeft;
        area = {
          x1: placeHolderLeft,
          x2: placeHolderLeft + tabDropPlaceholderRectWidth,
          y1: tabTop,
          y2: tabTop + tabElement.clientHeight
        };
      }
    }
    dropTargetIndicator.highlightArea(area, 0);
    return;
  }
  /** @internal */
  resetHeaderDropZone() {
    this.layoutManager.tabDropPlaceholder.remove();
  }
  /** @internal */
  setupHeaderPosition() {
    setElementDisplayVisibility(this._header.element, this._header.show);
    this.element.classList.remove(
      "lm_left",
      "lm_right",
      "lm_bottom"
      /* Bottom */
    );
    if (this._header.leftRightSided) {
      this.element.classList.add("lm_" + this._header.side);
    }
    this.updateSize(false);
  }
  /** @internal */
  highlightBodyDropZone(segment) {
    if (this._contentAreaDimensions === void 0) {
      throw new UnexpectedUndefinedError("SHBDZC82265");
    } else {
      const highlightArea = this._contentAreaDimensions[segment].highlightArea;
      const dropTargetIndicator = this.layoutManager.dropTargetIndicator;
      if (dropTargetIndicator === null) {
        throw new UnexpectedNullError("SHBDZD96110");
      } else {
        dropTargetIndicator.highlightArea(highlightArea, 1);
        this._dropSegment = segment;
      }
    }
  }
  /** @internal */
  handleResize() {
    this._header.updateTabSizes();
  }
  /** @internal */
  handleMaximised() {
    this._header.processMaximised();
  }
  /** @internal */
  handleMinimised() {
    this._header.processMinimised();
  }
  /** @internal */
  handlePopoutEvent() {
    this.popout();
  }
  /** @internal */
  handleHeaderClickEvent(ev) {
    const eventName = EventEmitter.headerClickEventName;
    const bubblingEvent = new EventEmitter.ClickBubblingEvent(eventName, this, ev);
    this.emit(eventName, bubblingEvent);
  }
  /** @internal */
  handleHeaderTouchStartEvent(ev) {
    const eventName = EventEmitter.headerTouchStartEventName;
    const bubblingEvent = new EventEmitter.TouchStartBubblingEvent(eventName, this, ev);
    this.emit(eventName, bubblingEvent);
  }
  /** @internal */
  handleHeaderComponentRemoveEvent(item) {
    this.removeChild(item, false);
  }
  /** @internal */
  handleHeaderComponentFocusEvent(item) {
    this.setActiveComponentItem(item, true);
  }
  /** @internal */
  handleHeaderComponentStartDragEvent(x, y, dragListener, componentItem) {
    if (this.isMaximised === true) {
      this.toggleMaximise();
    }
    this.layoutManager.startComponentDrag(x, y, dragListener, componentItem, this);
  }
  /** @internal */
  createHeaderConfig() {
    if (!this._headerSideChanged) {
      return ResolvedHeaderedItemConfig.Header.createCopy(this._headerConfig);
    } else {
      const show = this._header.show ? this._header.side : false;
      let result = ResolvedHeaderedItemConfig.Header.createCopy(this._headerConfig, show);
      if (result === void 0) {
        result = {
          show,
          popout: void 0,
          maximise: void 0,
          close: void 0,
          minimise: void 0,
          tabDropdown: void 0
        };
      }
      return result;
    }
  }
  /** @internal */
  emitStateChangedEvent() {
    this.emitBaseBubblingEvent("stateChanged");
  }
};
(function(Stack2) {
  function createElement(document2) {
    const element = document2.createElement("div");
    element.classList.add(
      "lm_item"
      /* Item */
    );
    element.classList.add(
      "lm_stack"
      /* Stack */
    );
    return element;
  }
  Stack2.createElement = createElement;
})(Stack || (Stack = {}));

// dist/esm/ts/controls/drag-proxy.js
var DragProxy = class extends EventEmitter {
  /**
   * @param x - The initial x position
   * @param y - The initial y position
   * @internal
   */
  constructor(x, y, _dragListener, _layoutManager, _componentItem, _originalParent) {
    super();
    this._dragListener = _dragListener;
    this._layoutManager = _layoutManager;
    this._componentItem = _componentItem;
    this._originalParent = _originalParent;
    this._area = null;
    this._lastValidArea = null;
    this._dragListener.on("drag", (offsetX, offsetY, event) => this.onDrag(offsetX, offsetY, event));
    this._dragListener.on("dragStop", () => this.onDrop());
    this.createDragProxyElements(x, y);
    if (this._componentItem.parent === null) {
      throw new UnexpectedNullError("DPC10097");
    }
    this._componentItemFocused = this._componentItem.focused;
    if (this._componentItemFocused) {
      this._componentItem.blur();
    }
    this._componentItem.parent.removeChild(this._componentItem, true);
    this.setDimensions();
    document.body.appendChild(this._element);
    this.determineMinMaxXY();
    this._layoutManager.calculateItemAreas();
    this.setDropPosition(x, y);
  }
  get element() {
    return this._element;
  }
  /** Create Stack-like structure to contain the dragged component */
  createDragProxyElements(initialX, initialY) {
    this._element = document.createElement("div");
    this._element.classList.add(
      "lm_dragProxy"
      /* DragProxy */
    );
    const headerElement = document.createElement("div");
    headerElement.classList.add(
      "lm_header"
      /* Header */
    );
    const tabsElement = document.createElement("div");
    tabsElement.classList.add(
      "lm_tabs"
      /* Tabs */
    );
    const tabElement = document.createElement("div");
    tabElement.classList.add(
      "lm_tab"
      /* Tab */
    );
    const titleElement = document.createElement("span");
    titleElement.classList.add(
      "lm_title"
      /* Title */
    );
    tabElement.appendChild(titleElement);
    tabsElement.appendChild(tabElement);
    headerElement.appendChild(tabsElement);
    this._proxyContainerElement = document.createElement("div");
    this._proxyContainerElement.classList.add(
      "lm_content"
      /* Content */
    );
    this._element.appendChild(headerElement);
    this._element.appendChild(this._proxyContainerElement);
    if (this._originalParent instanceof Stack && this._originalParent.headerShow) {
      this._sided = this._originalParent.headerLeftRightSided;
      this._element.classList.add("lm_" + this._originalParent.headerSide);
      if ([Side.right, Side.bottom].indexOf(this._originalParent.headerSide) >= 0) {
        this._proxyContainerElement.insertAdjacentElement("afterend", headerElement);
      }
    }
    this._element.style.left = numberToPixels(initialX);
    this._element.style.top = numberToPixels(initialY);
    tabElement.setAttribute("title", this._componentItem.title);
    titleElement.insertAdjacentText("afterbegin", this._componentItem.title);
    this._proxyContainerElement.appendChild(this._componentItem.element);
  }
  determineMinMaxXY() {
    const groundItem = this._layoutManager.groundItem;
    if (groundItem === void 0) {
      throw new UnexpectedUndefinedError("DPDMMXY73109");
    } else {
      const groundElement = groundItem.element;
      const rect = groundElement.getBoundingClientRect();
      this._minX = rect.left + document.body.scrollLeft;
      this._minY = rect.top + document.body.scrollTop;
      this._maxX = this._minX + rect.width;
      this._maxY = this._minY + rect.height;
    }
  }
  /**
   * Callback on every mouseMove event during a drag. Determines if the drag is
   * still within the valid drag area and calls the layoutManager to highlight the
   * current drop area
   *
   * @param offsetX - The difference from the original x position in px
   * @param offsetY - The difference from the original y position in px
   * @param event -
   * @internal
   */
  onDrag(offsetX, offsetY, event) {
    const x = event.pageX;
    const y = event.pageY;
    this.setDropPosition(x, y);
    this._componentItem.drag();
  }
  /**
   * Sets the target position, highlighting the appropriate area
   *
   * @param x - The x position in px
   * @param y - The y position in px
   *
   * @internal
   */
  setDropPosition(x, y) {
    if (this._layoutManager.layoutConfig.settings.constrainDragToContainer) {
      if (x <= this._minX) {
        x = Math.ceil(this._minX);
      } else if (x >= this._maxX) {
        x = Math.floor(this._maxX);
      }
      if (y <= this._minY) {
        y = Math.ceil(this._minY);
      } else if (y >= this._maxY) {
        y = Math.floor(this._maxY);
      }
    }
    this._element.style.left = numberToPixels(x);
    this._element.style.top = numberToPixels(y);
    this._area = this._layoutManager.getArea(x, y);
    if (this._area !== null) {
      this._lastValidArea = this._area;
      this._area.contentItem.highlightDropZone(x, y, this._area);
    }
  }
  /**
   * Callback when the drag has finished. Determines the drop area
   * and adds the child to it
   * @internal
   */
  onDrop() {
    const dropTargetIndicator = this._layoutManager.dropTargetIndicator;
    if (dropTargetIndicator === null) {
      throw new UnexpectedNullError("DPOD30011");
    } else {
      dropTargetIndicator.hide();
    }
    this._componentItem.exitDragMode();
    let droppedComponentItem;
    if (this._area !== null) {
      droppedComponentItem = this._componentItem;
      this._area.contentItem.onDrop(droppedComponentItem, this._area);
    } else if (this._lastValidArea !== null) {
      droppedComponentItem = this._componentItem;
      const newParentContentItem = this._lastValidArea.contentItem;
      newParentContentItem.onDrop(droppedComponentItem, this._lastValidArea);
    } else if (this._originalParent) {
      droppedComponentItem = this._componentItem;
      this._originalParent.addChild(droppedComponentItem);
    } else {
      this._componentItem.destroy();
    }
    this._element.remove();
    this._layoutManager.emit("itemDropped", this._componentItem);
    if (this._componentItemFocused && droppedComponentItem !== void 0) {
      droppedComponentItem.focus();
    }
  }
  /**
   * Updates the Drag Proxy's dimensions
   * @internal
   */
  setDimensions() {
    const dimensions = this._layoutManager.layoutConfig.dimensions;
    if (dimensions === void 0) {
      throw new Error("DragProxy.setDimensions: dimensions undefined");
    }
    let width = dimensions.dragProxyWidth;
    let height = dimensions.dragProxyHeight;
    if (width === void 0 || height === void 0) {
      throw new Error("DragProxy.setDimensions: width and/or height undefined");
    }
    const headerHeight = this._layoutManager.layoutConfig.header.show === false ? 0 : dimensions.headerHeight;
    this._element.style.width = numberToPixels(width);
    this._element.style.height = numberToPixels(height);
    width -= this._sided ? headerHeight : 0;
    height -= !this._sided ? headerHeight : 0;
    this._proxyContainerElement.style.width = numberToPixels(width);
    this._proxyContainerElement.style.height = numberToPixels(height);
    this._componentItem.enterDragMode(width, height);
    this._componentItem.show();
  }
};

// dist/esm/ts/controls/drag-source.js
var DragSource = class _DragSource {
  /** @internal */
  constructor(_layoutManager, _element, _extraAllowableChildTargets, _componentTypeOrFtn, _componentState, _title, _id) {
    this._layoutManager = _layoutManager;
    this._element = _element;
    this._extraAllowableChildTargets = _extraAllowableChildTargets;
    this._componentTypeOrFtn = _componentTypeOrFtn;
    this._componentState = _componentState;
    this._title = _title;
    this._id = _id;
    this._dragListener = null;
    this._dummyGroundContainer = document.createElement("div");
    const dummyRootItemConfig = ResolvedRowOrColumnItemConfig.createDefault("row");
    this._dummyGroundContentItem = new GroundItem(this._layoutManager, dummyRootItemConfig, this._dummyGroundContainer);
    this.createDragListener();
  }
  /**
   * Disposes of the drag listeners so the drag source is not usable any more.
   * @internal
   */
  destroy() {
    this.removeDragListener();
  }
  /**
   * Called initially and after every drag
   * @internal
   */
  createDragListener() {
    this.removeDragListener();
    this._dragListener = new DragListener(this._element, this._extraAllowableChildTargets);
    this._dragListener.on("dragStart", (x, y) => this.onDragStart(x, y));
    this._dragListener.on("dragStop", () => this.onDragStop());
  }
  /**
   * Callback for the DragListener's dragStart event
   *
   * @param x - The x position of the mouse on dragStart
   * @param y - The x position of the mouse on dragStart
   * @internal
   */
  onDragStart(x, y) {
    var _a;
    const type = "component";
    let dragSourceItemConfig;
    if (typeof this._componentTypeOrFtn === "function") {
      const ftnDragSourceItemConfig = this._componentTypeOrFtn();
      if (_DragSource.isDragSourceComponentItemConfig(ftnDragSourceItemConfig)) {
        dragSourceItemConfig = {
          type,
          componentState: ftnDragSourceItemConfig.state,
          componentType: ftnDragSourceItemConfig.type,
          title: (_a = ftnDragSourceItemConfig.title) !== null && _a !== void 0 ? _a : this._title
        };
      } else {
        dragSourceItemConfig = ftnDragSourceItemConfig;
      }
    } else {
      dragSourceItemConfig = {
        type,
        componentState: this._componentState,
        componentType: this._componentTypeOrFtn,
        title: this._title,
        id: this._id
      };
    }
    const resolvedItemConfig = ComponentItemConfig.resolve(dragSourceItemConfig, false);
    const componentItem = new ComponentItem(this._layoutManager, resolvedItemConfig, this._dummyGroundContentItem);
    this._dummyGroundContentItem.contentItems.push(componentItem);
    if (this._dragListener === null) {
      throw new UnexpectedNullError("DSODSD66746");
    } else {
      const dragProxy = new DragProxy(x, y, this._dragListener, this._layoutManager, componentItem, this._dummyGroundContentItem);
      const transitionIndicator = this._layoutManager.transitionIndicator;
      if (transitionIndicator === null) {
        throw new UnexpectedNullError("DSODST66746");
      } else {
        transitionIndicator.transitionElements(this._element, dragProxy.element);
      }
    }
  }
  /** @internal */
  onDragStop() {
    this.createDragListener();
  }
  /**
   * Called after every drag and when the drag source is being disposed of.
   * @internal
   */
  removeDragListener() {
    if (this._dragListener !== null) {
      this._dragListener.destroy();
      this._dragListener = null;
    }
  }
};
(function(DragSource2) {
  function isDragSourceComponentItemConfig(config) {
    return !("componentType" in config);
  }
  DragSource2.isDragSourceComponentItemConfig = isDragSourceComponentItemConfig;
})(DragSource || (DragSource = {}));

// dist/esm/ts/controls/drop-target-indicator.js
var DropTargetIndicator = class {
  constructor() {
    this._element = document.createElement("div");
    this._element.classList.add(
      "lm_dropTargetIndicator"
      /* DropTargetIndicator */
    );
    const innerElement = document.createElement("div");
    innerElement.classList.add(
      "lm_inner"
      /* Inner */
    );
    this._element.appendChild(innerElement);
    document.body.appendChild(this._element);
  }
  destroy() {
    this._element.remove();
  }
  highlightArea(area, margin) {
    this._element.style.left = numberToPixels(area.x1 + margin);
    this._element.style.top = numberToPixels(area.y1 + margin);
    this._element.style.width = numberToPixels(area.x2 - area.x1 - margin);
    this._element.style.height = numberToPixels(area.y2 - area.y1 - margin);
    this._element.style.display = "block";
  }
  hide() {
    setElementDisplayVisibility(this._element, false);
  }
};

// dist/esm/ts/controls/transition-indicator.js
var TransitionIndicator = class {
  constructor() {
    this._element = document.createElement("div");
    this._element.classList.add(
      "lm_transition_indicator"
      /* TransitionIndicator */
    );
    document.body.appendChild(this._element);
    this._toElement = null;
    this._fromDimensions = null;
    this._totalAnimationDuration = 200;
    this._animationStartTime = null;
  }
  destroy() {
    this._element.remove();
  }
  // eslint-disable-next-line @typescript-eslint/no-unused-vars
  transitionElements(fromElement, toElement) {
    return;
  }
  nextAnimationFrame() {
  }
  measure(element) {
    const rect = element.getBoundingClientRect();
    return {
      left: rect.left,
      top: rect.top,
      width: element.offsetWidth,
      height: element.offsetHeight
    };
  }
};

// dist/esm/ts/utils/event-hub.js
var EventHub = class _EventHub extends EventEmitter {
  /**
   * Creates a new EventHub instance
   * @param _layoutManager - the layout manager to synchronize between the windows
   * @internal
   */
  constructor(_layoutManager) {
    super();
    this._layoutManager = _layoutManager;
    this._childEventListener = (childEvent) => this.onEventFromChild(childEvent);
    globalThis.addEventListener(_EventHub.ChildEventName, this._childEventListener, { passive: true });
  }
  /**
   * Emit an event and notify listeners
   *
   * @param eventName - The name of the event
   * @param args - Additional arguments that will be passed to the listener
   * @public
   */
  emit(eventName, ...args) {
    if (eventName === "userBroadcast") {
      this.emitUserBroadcast(...args);
    } else {
      super.emit(eventName, ...args);
    }
  }
  /**
   * Broadcasts a message to all other currently opened windows.
   * @public
   */
  emitUserBroadcast(...args) {
    this.handleUserBroadcastEvent("userBroadcast", args);
  }
  /**
   * Destroys the EventHub
   * @internal
   */
  destroy() {
    globalThis.removeEventListener(_EventHub.ChildEventName, this._childEventListener);
  }
  /**
   * Internal processor to process local events.
   * @internal
   */
  handleUserBroadcastEvent(eventName, args) {
    if (this._layoutManager.isSubWindow) {
      this.propagateToParent(eventName, args);
    } else {
      this.propagateToThisAndSubtree(eventName, args);
    }
  }
  /**
   * Callback for child events raised on the window
   * @internal
   */
  onEventFromChild(event) {
    const detail = event.detail;
    this.handleUserBroadcastEvent(detail.eventName, detail.args);
  }
  /**
   * Propagates the event to the parent by emitting
   * it on the parent's DOM window
   * @internal
   */
  propagateToParent(eventName, args) {
    const detail = {
      layoutManager: this._layoutManager,
      eventName,
      args
    };
    const eventInit = {
      bubbles: true,
      cancelable: true,
      detail
    };
    const event = new CustomEvent(_EventHub.ChildEventName, eventInit);
    const opener = globalThis.opener;
    if (opener === null) {
      throw new UnexpectedNullError("EHPTP15778");
    }
    opener.dispatchEvent(event);
  }
  /**
   * Propagate events to the whole subtree under this event hub.
   * @internal
   */
  propagateToThisAndSubtree(eventName, args) {
    this.emitUnknown(eventName, ...args);
    for (let i = 0; i < this._layoutManager.openPopouts.length; i++) {
      const childGl = this._layoutManager.openPopouts[i].getGlInstance();
      if (childGl) {
        childGl.eventHub.propagateToThisAndSubtree(eventName, args);
      }
    }
  }
};
(function(EventHub2) {
  EventHub2.ChildEventName = "gl_child_event";
})(EventHub || (EventHub = {}));

// dist/esm/ts/layout-manager.js
var LayoutManager = class _LayoutManager extends EventEmitter {
  /**
  * @param container - A Dom HTML element. Defaults to body
  * @internal
  */
  constructor(parameters) {
    super();
    this.resizeWithContainerAutomatically = false;
    this.resizeDebounceInterval = 100;
    this.resizeDebounceExtendedWhenPossible = true;
    this._isInitialised = false;
    this._groundItem = void 0;
    this._openPopouts = [];
    this._dropTargetIndicator = null;
    this._transitionIndicator = null;
    this._itemAreas = [];
    this._maximisePlaceholder = _LayoutManager.createMaximisePlaceElement(document);
    this._tabDropPlaceholder = _LayoutManager.createTabDropPlaceholderElement(document);
    this._dragSources = [];
    this._updatingColumnsResponsive = false;
    this._firstLoad = true;
    this._eventHub = new EventHub(this);
    this._width = null;
    this._height = null;
    this._virtualSizedContainers = [];
    this._virtualSizedContainerAddingBeginCount = 0;
    this._sizeInvalidationBeginCount = 0;
    this._resizeObserver = new ResizeObserver(() => this.handleContainerResize());
    this._windowBeforeUnloadListener = () => this.onBeforeUnload();
    this._windowBeforeUnloadListening = false;
    this._maximisedStackBeforeDestroyedListener = (ev) => this.cleanupBeforeMaximisedStackDestroyed(ev);
    this.isSubWindow = parameters.isSubWindow;
    this._constructorOrSubWindowLayoutConfig = parameters.constructorOrSubWindowLayoutConfig;
    I18nStrings.checkInitialise();
    ConfigMinifier.checkInitialise();
    if (parameters.containerElement !== void 0) {
      this._containerElement = parameters.containerElement;
    }
  }
  get container() {
    return this._containerElement;
  }
  get isInitialised() {
    return this._isInitialised;
  }
  /** @internal */
  get groundItem() {
    return this._groundItem;
  }
  /** @internal @deprecated use {@link (LayoutManager:class).groundItem} instead */
  get root() {
    return this._groundItem;
  }
  get openPopouts() {
    return this._openPopouts;
  }
  /** @internal */
  get dropTargetIndicator() {
    return this._dropTargetIndicator;
  }
  /** @internal @deprecated To be removed */
  get transitionIndicator() {
    return this._transitionIndicator;
  }
  get width() {
    return this._width;
  }
  get height() {
    return this._height;
  }
  /**
   * Retrieves the {@link (EventHub:class)} instance associated with this layout manager.
   * This can be used to propagate events between the windows
   * @public
   */
  get eventHub() {
    return this._eventHub;
  }
  get rootItem() {
    if (this._groundItem === void 0) {
      throw new Error("Cannot access rootItem before init");
    } else {
      const groundContentItems = this._groundItem.contentItems;
      if (groundContentItems.length === 0) {
        return void 0;
      } else {
        return this._groundItem.contentItems[0];
      }
    }
  }
  get focusedComponentItem() {
    return this._focusedComponentItem;
  }
  /** @internal */
  get tabDropPlaceholder() {
    return this._tabDropPlaceholder;
  }
  get maximisedStack() {
    return this._maximisedStack;
  }
  /** @deprecated indicates deprecated constructor use */
  get deprecatedConstructor() {
    return !this.isSubWindow && this._constructorOrSubWindowLayoutConfig !== void 0;
  }
  /**
   * Destroys the LayoutManager instance itself as well as every ContentItem
   * within it. After this is called nothing should be left of the LayoutManager.
   *
   * This function only needs to be called if an application wishes to destroy the Golden Layout object while
   * a page remains loaded. When a page is unloaded, all resources claimed by Golden Layout will automatically
   * be released.
   */
  destroy() {
    if (this._isInitialised) {
      if (this._windowBeforeUnloadListening) {
        globalThis.removeEventListener("beforeunload", this._windowBeforeUnloadListener);
        this._windowBeforeUnloadListening = false;
      }
      if (this.layoutConfig.settings.closePopoutsOnUnload === true) {
        this.closeAllOpenPopouts();
      }
      this._resizeObserver.disconnect();
      this.checkClearResizeTimeout();
      if (this._groundItem !== void 0) {
        this._groundItem.destroy();
      }
      this._tabDropPlaceholder.remove();
      if (this._dropTargetIndicator !== null) {
        this._dropTargetIndicator.destroy();
      }
      if (this._transitionIndicator !== null) {
        this._transitionIndicator.destroy();
      }
      this._eventHub.destroy();
      for (const dragSource of this._dragSources) {
        dragSource.destroy();
      }
      this._dragSources = [];
      this._isInitialised = false;
    }
  }
  /**
   * Takes a GoldenLayout configuration object and
   * replaces its keys and values recursively with
   * one letter codes
   * @deprecated use {@link (ResolvedLayoutConfig:namespace).minifyConfig} instead
   */
  minifyConfig(config) {
    return ResolvedLayoutConfig.minifyConfig(config);
  }
  /**
   * Takes a configuration Object that was previously minified
   * using minifyConfig and returns its original version
   * @deprecated use {@link (ResolvedLayoutConfig:namespace).unminifyConfig} instead
   */
  unminifyConfig(config) {
    return ResolvedLayoutConfig.unminifyConfig(config);
  }
  /**
   * Called from GoldenLayout class. Finishes of init
   * @internal
   */
  init() {
    this.setContainer();
    this._dropTargetIndicator = new DropTargetIndicator(
      /*this.container*/
    );
    this._transitionIndicator = new TransitionIndicator();
    this.updateSizeFromContainer();
    let subWindowRootConfig;
    if (this.isSubWindow) {
      if (this._constructorOrSubWindowLayoutConfig === void 0) {
        throw new UnexpectedUndefinedError("LMIU07155");
      } else {
        const root = this._constructorOrSubWindowLayoutConfig.root;
        if (root === void 0) {
          throw new AssertError("LMIC07156");
        } else {
          if (ItemConfig.isComponent(root)) {
            subWindowRootConfig = root;
          } else {
            throw new AssertError("LMIC07157");
          }
        }
        const resolvedLayoutConfig = LayoutConfig.resolve(this._constructorOrSubWindowLayoutConfig);
        this.layoutConfig = Object.assign(Object.assign({}, resolvedLayoutConfig), { root: void 0 });
      }
    } else {
      if (this._constructorOrSubWindowLayoutConfig === void 0) {
        this.layoutConfig = ResolvedLayoutConfig.createDefault();
      } else {
        this.layoutConfig = LayoutConfig.resolve(this._constructorOrSubWindowLayoutConfig);
      }
    }
    const layoutConfig = this.layoutConfig;
    this._groundItem = new GroundItem(this, layoutConfig.root, this._containerElement);
    this._groundItem.init();
    this.checkLoadedLayoutMaximiseItem();
    this._resizeObserver.observe(this._containerElement);
    this._isInitialised = true;
    this.adjustColumnsResponsive();
    this.emit("initialised");
    if (subWindowRootConfig !== void 0) {
      this.loadComponentAsRoot(subWindowRootConfig);
    }
  }
  /**
   * Loads a new layout
   * @param layoutConfig - New layout to be loaded
   */
  loadLayout(layoutConfig) {
    if (!this.isInitialised) {
      throw new Error("GoldenLayout: Need to call init() if LayoutConfig with defined root passed to constructor");
    } else {
      if (this._groundItem === void 0) {
        throw new UnexpectedUndefinedError("LMLL11119");
      } else {
        this.createSubWindows();
        this.layoutConfig = LayoutConfig.resolve(layoutConfig);
        this._groundItem.loadRoot(this.layoutConfig.root);
        this.checkLoadedLayoutMaximiseItem();
        this.adjustColumnsResponsive();
      }
    }
  }
  /**
   * Creates a layout configuration object based on the the current state
   *
   * @public
   * @returns GoldenLayout configuration
   */
  saveLayout() {
    if (this._isInitialised === false) {
      throw new Error("Can't create config, layout not yet initialised");
    } else {
      if (this._groundItem === void 0) {
        throw new UnexpectedUndefinedError("LMTC18244");
      } else {
        const groundContent = this._groundItem.calculateConfigContent();
        let rootItemConfig;
        if (groundContent.length !== 1) {
          rootItemConfig = void 0;
        } else {
          rootItemConfig = groundContent[0];
        }
        this.reconcilePopoutWindows();
        const openPopouts = [];
        for (let i = 0; i < this._openPopouts.length; i++) {
          openPopouts.push(this._openPopouts[i].toConfig());
        }
        const config = {
          root: rootItemConfig,
          openPopouts,
          settings: ResolvedLayoutConfig.Settings.createCopy(this.layoutConfig.settings),
          dimensions: ResolvedLayoutConfig.Dimensions.createCopy(this.layoutConfig.dimensions),
          header: ResolvedLayoutConfig.Header.createCopy(this.layoutConfig.header),
          resolved: true
        };
        return config;
      }
    }
  }
  /**
   * Removes any existing layout. Effectively, an empty layout will be loaded.
   */
  clear() {
    if (this._groundItem === void 0) {
      throw new UnexpectedUndefinedError("LMCL11129");
    } else {
      this._groundItem.clearRoot();
    }
  }
  /**
   * @deprecated Use {@link (LayoutManager:class).saveLayout}
   */
  toConfig() {
    return this.saveLayout();
  }
  /**
   * Adds a new ComponentItem.  Will use default location selectors to ensure a location is found and
   * component is successfully added
   * @param componentTypeName - Name of component type to be created.
   * @param state - Optional initial state to be assigned to component
   * @returns New ComponentItem created.
   */
  newComponent(componentType, componentState, title) {
    const componentItem = this.newComponentAtLocation(componentType, componentState, title);
    if (componentItem === void 0) {
      throw new AssertError("LMNC65588");
    } else {
      return componentItem;
    }
  }
  /**
   * Adds a ComponentItem at the first valid selector location.
   * @param componentTypeName - Name of component type to be created.
   * @param state - Optional initial state to be assigned to component
   * @param locationSelectors - Array of location selectors used to find location in layout where component
   * will be added. First location in array which is valid will be used. If locationSelectors is undefined,
   * {@link (LayoutManager:namespace).defaultLocationSelectors} will be used
   * @returns New ComponentItem created or undefined if no valid location selector was in array.
   */
  newComponentAtLocation(componentType, componentState, title, locationSelectors) {
    if (this._groundItem === void 0) {
      throw new Error("Cannot add component before init");
    } else {
      const location2 = this.addComponentAtLocation(componentType, componentState, title, locationSelectors);
      if (location2 === void 0) {
        return void 0;
      } else {
        const createdItem = location2.parentItem.contentItems[location2.index];
        if (!ContentItem.isComponentItem(createdItem)) {
          throw new AssertError("LMNC992877533");
        } else {
          return createdItem;
        }
      }
    }
  }
  /**
   * Adds a new ComponentItem.  Will use default location selectors to ensure a location is found and
   * component is successfully added
   * @param componentType - Type of component to be created.
   * @param state - Optional initial state to be assigned to component
   * @returns Location of new ComponentItem created.
   */
  addComponent(componentType, componentState, title) {
    const location2 = this.addComponentAtLocation(componentType, componentState, title);
    if (location2 === void 0) {
      throw new AssertError("LMAC99943");
    } else {
      return location2;
    }
  }
  /**
   * Adds a ComponentItem at the first valid selector location.
   * @param componentType - Type of component to be created.
   * @param state - Optional initial state to be assigned to component
   * @param locationSelectors - Array of location selectors used to find determine location in layout where component
   * will be added. First location in array which is valid will be used. If undefined,
   * {@link (LayoutManager:namespace).defaultLocationSelectors} will be used.
   * @returns Location of new ComponentItem created or undefined if no valid location selector was in array.
   */
  addComponentAtLocation(componentType, componentState, title, locationSelectors) {
    const itemConfig = {
      type: "component",
      componentType,
      componentState,
      title
    };
    return this.addItemAtLocation(itemConfig, locationSelectors);
  }
  /**
   * Adds a new ContentItem.  Will use default location selectors to ensure a location is found and
   * component is successfully added
   * @param itemConfig - ResolvedItemConfig of child to be added.
   * @returns New ContentItem created.
  */
  newItem(itemConfig) {
    const contentItem = this.newItemAtLocation(itemConfig);
    if (contentItem === void 0) {
      throw new AssertError("LMNC65588");
    } else {
      return contentItem;
    }
  }
  /**
   * Adds a new child ContentItem under the root ContentItem.  If a root does not exist, then create root ContentItem instead
   * @param itemConfig - ResolvedItemConfig of child to be added.
   * @param locationSelectors - Array of location selectors used to find determine location in layout where ContentItem
   * will be added. First location in array which is valid will be used. If undefined,
   * {@link (LayoutManager:namespace).defaultLocationSelectors} will be used.
   * @returns New ContentItem created or undefined if no valid location selector was in array. */
  newItemAtLocation(itemConfig, locationSelectors) {
    if (this._groundItem === void 0) {
      throw new Error("Cannot add component before init");
    } else {
      const location2 = this.addItemAtLocation(itemConfig, locationSelectors);
      if (location2 === void 0) {
        return void 0;
      } else {
        const createdItem = location2.parentItem.contentItems[location2.index];
        return createdItem;
      }
    }
  }
  /**
   * Adds a new ContentItem.  Will use default location selectors to ensure a location is found and
   * component is successfully added.
   * @param itemConfig - ResolvedItemConfig of child to be added.
   * @returns Location of new ContentItem created. */
  addItem(itemConfig) {
    const location2 = this.addItemAtLocation(itemConfig);
    if (location2 === void 0) {
      throw new AssertError("LMAI99943");
    } else {
      return location2;
    }
  }
  /**
   * Adds a ContentItem at the first valid selector location.
   * @param itemConfig - ResolvedItemConfig of child to be added.
   * @param locationSelectors - Array of location selectors used to find determine location in layout where ContentItem
   * will be added. First location in array which is valid will be used. If undefined,
   * {@link (LayoutManager:namespace).defaultLocationSelectors} will be used.
   * @returns Location of new ContentItem created or undefined if no valid location selector was in array. */
  addItemAtLocation(itemConfig, locationSelectors) {
    if (this._groundItem === void 0) {
      throw new Error("Cannot add component before init");
    } else {
      if (locationSelectors === void 0) {
        locationSelectors = _LayoutManager.defaultLocationSelectors;
      }
      const location2 = this.findFirstLocation(locationSelectors);
      if (location2 === void 0) {
        return void 0;
      } else {
        let parentItem = location2.parentItem;
        let addIdx;
        switch (parentItem.type) {
          case ItemType.ground: {
            const groundItem = parentItem;
            addIdx = groundItem.addItem(itemConfig, location2.index);
            if (addIdx >= 0) {
              parentItem = this._groundItem.contentItems[0];
            } else {
              addIdx = 0;
            }
            break;
          }
          case ItemType.row:
          case ItemType.column: {
            const rowOrColumn = parentItem;
            addIdx = rowOrColumn.addItem(itemConfig, location2.index);
            break;
          }
          case ItemType.stack: {
            if (!ItemConfig.isComponent(itemConfig)) {
              throw Error(i18nStrings[
                6
                /* ItemConfigIsNotTypeComponent */
              ]);
            } else {
              const stack = parentItem;
              addIdx = stack.addItem(itemConfig, location2.index);
              break;
            }
          }
          case ItemType.component: {
            throw new AssertError("LMAIALC87444602");
          }
          default:
            throw new UnreachableCaseError("LMAIALU98881733", parentItem.type);
        }
        if (ItemConfig.isComponent(itemConfig)) {
          const item = parentItem.contentItems[addIdx];
          if (ContentItem.isStack(item)) {
            parentItem = item;
            addIdx = 0;
          }
        }
        location2.parentItem = parentItem;
        location2.index = addIdx;
        return location2;
      }
    }
  }
  /** Loads the specified component ResolvedItemConfig as root.
   * This can be used to display a Component all by itself.  The layout cannot be changed other than having another new layout loaded.
   * Note that, if this layout is saved and reloaded, it will reload with the Component as a child of a Stack.
  */
  loadComponentAsRoot(itemConfig) {
    if (this._groundItem === void 0) {
      throw new Error("Cannot add item before init");
    } else {
      this._groundItem.loadComponentAsRoot(itemConfig);
    }
  }
  /** @deprecated Use {@link (LayoutManager:class).setSize} */
  updateSize(width, height) {
    this.setSize(width, height);
  }
  /**
   * Updates the layout managers size
   *
   * @param width - Width in pixels
   * @param height - Height in pixels
   */
  setSize(width, height) {
    this._width = width;
    this._height = height;
    if (this._isInitialised === true) {
      if (this._groundItem === void 0) {
        throw new UnexpectedUndefinedError("LMUS18881");
      } else {
        this._groundItem.setSize(this._width, this._height);
        if (this._maximisedStack) {
          const { width: width2, height: height2 } = getElementWidthAndHeight(this._containerElement);
          setElementWidth(this._maximisedStack.element, width2);
          setElementHeight(this._maximisedStack.element, height2);
          this._maximisedStack.updateSize(false);
        }
        this.adjustColumnsResponsive();
      }
    }
  }
  /** @internal */
  beginSizeInvalidation() {
    this._sizeInvalidationBeginCount++;
  }
  /** @internal */
  endSizeInvalidation() {
    if (--this._sizeInvalidationBeginCount === 0) {
      this.updateSizeFromContainer();
    }
  }
  /** @internal */
  updateSizeFromContainer() {
    const { width, height } = getElementWidthAndHeight(this._containerElement);
    this.setSize(width, height);
  }
  /**
   * Update the size of the root ContentItem.  This will update the size of all contentItems in the tree
   * @param force - In some cases the size is not updated if it has not changed. In this case, events
   * (such as ComponentContainer.virtualRectingRequiredEvent) are not fired. Setting force to true, ensures the size is updated regardless, and
   * the respective events are fired. This is sometimes necessary when a component's size has not changed but it has become visible, and the
   * relevant events need to be fired.
   */
  updateRootSize(force = false) {
    if (this._groundItem === void 0) {
      throw new UnexpectedUndefinedError("LMURS28881");
    } else {
      this._groundItem.updateSize(force);
    }
  }
  /** @public */
  createAndInitContentItem(config, parent) {
    const newItem = this.createContentItem(config, parent);
    newItem.init();
    return newItem;
  }
  /**
   * Recursively creates new item tree structures based on a provided
   * ItemConfiguration object
   *
   * @param config - ResolvedItemConfig
   * @param parent - The item the newly created item should be a child of
   * @internal
   */
  createContentItem(config, parent) {
    if (typeof config.type !== "string") {
      throw new ConfigurationError("Missing parameter 'type'", JSON.stringify(config));
    }
    if (
      // If this is a component
      ResolvedItemConfig.isComponentItem(config) && // and it's not already within a stack
      !(parent instanceof Stack) && // and we have a parent
      !!parent && // and it's not the topmost item in a new window
      !(this.isSubWindow === true && parent instanceof GroundItem)
    ) {
      const stackConfig = {
        type: ItemType.stack,
        content: [config],
        size: config.size,
        sizeUnit: config.sizeUnit,
        minSize: config.minSize,
        minSizeUnit: config.minSizeUnit,
        id: config.id,
        maximised: config.maximised,
        isClosable: config.isClosable,
        activeItemIndex: 0,
        header: void 0
      };
      config = stackConfig;
    }
    const contentItem = this.createContentItemFromConfig(config, parent);
    return contentItem;
  }
  findFirstComponentItemById(id) {
    if (this._groundItem === void 0) {
      throw new UnexpectedUndefinedError("LMFFCIBI82446");
    } else {
      return this.findFirstContentItemTypeByIdRecursive(ItemType.component, id, this._groundItem);
    }
  }
  /**
   * Creates a popout window with the specified content at the specified position
   *
   * @param itemConfigOrContentItem - The content of the popout window's layout manager derived from either
   * a {@link (ContentItem:class)} or {@link (ItemConfig:interface)} or ResolvedItemConfig content (array of {@link (ItemConfig:interface)})
   * @param positionAndSize - The width, height, left and top of Popout window
   * @param parentId -The id of the element this item will be appended to when popIn is called
   * @param indexInParent - The position of this item within its parent element
   */
  createPopout(itemConfigOrContentItem, positionAndSize, parentId, indexInParent) {
    if (itemConfigOrContentItem instanceof ContentItem) {
      return this.createPopoutFromContentItem(itemConfigOrContentItem, positionAndSize, parentId, indexInParent);
    } else {
      return this.createPopoutFromItemConfig(itemConfigOrContentItem, positionAndSize, parentId, indexInParent);
    }
  }
  /** @internal */
  createPopoutFromContentItem(item, window2, parentId, indexInParent) {
    let parent = item.parent;
    let child = item;
    while (parent !== null && parent.contentItems.length === 1 && !parent.isGround) {
      child = parent;
      parent = parent.parent;
    }
    if (parent === null) {
      throw new UnexpectedNullError("LMCPFCI00834");
    } else {
      if (indexInParent === void 0) {
        indexInParent = parent.contentItems.indexOf(child);
      }
      if (parentId !== null) {
        parent.addPopInParentId(parentId);
      }
      if (window2 === void 0) {
        const windowLeft = globalThis.screenX || globalThis.screenLeft;
        const windowTop = globalThis.screenY || globalThis.screenTop;
        const offsetLeft = item.element.offsetLeft;
        const offsetTop = item.element.offsetTop;
        const { width, height } = getElementWidthAndHeight(item.element);
        window2 = {
          left: windowLeft + offsetLeft,
          top: windowTop + offsetTop,
          width,
          height
        };
      }
      const itemConfig = item.toConfig();
      item.remove();
      if (!ResolvedRootItemConfig.isRootItemConfig(itemConfig)) {
        throw new Error(`${i18nStrings[
          0
          /* PopoutCannotBeCreatedWithGroundItemConfig */
        ]}`);
      } else {
        return this.createPopoutFromItemConfig(itemConfig, window2, parentId, indexInParent);
      }
    }
  }
  /** @internal */
  beginVirtualSizedContainerAdding() {
    if (++this._virtualSizedContainerAddingBeginCount === 0) {
      this._virtualSizedContainers.length = 0;
    }
  }
  /** @internal */
  addVirtualSizedContainer(container) {
    this._virtualSizedContainers.push(container);
  }
  /** @internal */
  endVirtualSizedContainerAdding() {
    if (--this._virtualSizedContainerAddingBeginCount === 0) {
      const count = this._virtualSizedContainers.length;
      if (count > 0) {
        this.fireBeforeVirtualRectingEvent(count);
        for (let i = 0; i < count; i++) {
          const container = this._virtualSizedContainers[i];
          container.notifyVirtualRectingRequired();
        }
        this.fireAfterVirtualRectingEvent();
        this._virtualSizedContainers.length = 0;
      }
    }
  }
  /** @internal */
  fireBeforeVirtualRectingEvent(count) {
    if (this.beforeVirtualRectingEvent !== void 0) {
      this.beforeVirtualRectingEvent(count);
    }
  }
  /** @internal */
  fireAfterVirtualRectingEvent() {
    if (this.afterVirtualRectingEvent !== void 0) {
      this.afterVirtualRectingEvent();
    }
  }
  /** @internal */
  createPopoutFromItemConfig(rootItemConfig, window2, parentId, indexInParent) {
    const layoutConfig = this.toConfig();
    const popoutLayoutConfig = {
      root: rootItemConfig,
      openPopouts: [],
      settings: layoutConfig.settings,
      dimensions: layoutConfig.dimensions,
      header: layoutConfig.header,
      window: window2,
      parentId,
      indexInParent,
      resolved: true
    };
    return this.createPopoutFromPopoutLayoutConfig(popoutLayoutConfig);
  }
  /** @internal */
  createPopoutFromPopoutLayoutConfig(config) {
    var _a, _b, _c, _d;
    const configWindow = config.window;
    const initialWindow = {
      left: (_a = configWindow.left) !== null && _a !== void 0 ? _a : globalThis.screenX || globalThis.screenLeft + 20,
      top: (_b = configWindow.top) !== null && _b !== void 0 ? _b : globalThis.screenY || globalThis.screenTop + 20,
      width: (_c = configWindow.width) !== null && _c !== void 0 ? _c : 500,
      height: (_d = configWindow.height) !== null && _d !== void 0 ? _d : 309
    };
    const browserPopout = new BrowserPopout(config, initialWindow, this);
    browserPopout.on("initialised", () => this.emit("windowOpened", browserPopout));
    browserPopout.on("closed", () => this.reconcilePopoutWindows());
    this._openPopouts.push(browserPopout);
    if (this.layoutConfig.settings.closePopoutsOnUnload && !this._windowBeforeUnloadListening) {
      globalThis.addEventListener("beforeunload", this._windowBeforeUnloadListener, { passive: true });
      this._windowBeforeUnloadListening = true;
    }
    return browserPopout;
  }
  /**
   * Closes all Open Popouts
   * Applications can call this method when a page is unloaded to remove its open popouts
   */
  closeAllOpenPopouts() {
    for (let i = 0; i < this._openPopouts.length; i++) {
      this._openPopouts[i].close();
    }
    this._openPopouts.length = 0;
    if (this._windowBeforeUnloadListening) {
      globalThis.removeEventListener("beforeunload", this._windowBeforeUnloadListener);
      this._windowBeforeUnloadListening = false;
    }
  }
  newDragSource(element, componentTypeOrItemConfigCallback, componentState, title, id) {
    const dragSource = new DragSource(this, element, [], componentTypeOrItemConfigCallback, componentState, title, id);
    this._dragSources.push(dragSource);
    return dragSource;
  }
  /**
   * Removes a DragListener added by createDragSource() so the corresponding
   * DOM element is not a drag source any more.
   */
  removeDragSource(dragSource) {
    removeFromArray(dragSource, this._dragSources);
    dragSource.destroy();
  }
  /** @internal */
  startComponentDrag(x, y, dragListener, componentItem, stack) {
    new DragProxy(x, y, dragListener, this, componentItem, stack);
  }
  /**
   * Programmatically focuses an item. This focuses the specified component item
   * and the item emits a focus event
   *
   * @param item - The component item to be focused
   * @param suppressEvent - Whether to emit focus event
   */
  focusComponent(item, suppressEvent = false) {
    item.focus(suppressEvent);
  }
  /**
   * Programmatically blurs (defocuses) the currently focused component.
   * If a component item is focused, then it is blurred and and the item emits a blur event
   *
   * @param item - The component item to be blurred
   * @param suppressEvent - Whether to emit blur event
   */
  clearComponentFocus(suppressEvent = false) {
    this.setFocusedComponentItem(void 0, suppressEvent);
  }
  /**
   * Programmatically focuses a component item or removes focus (blurs) from an existing focused component item.
   *
   * @param item - If defined, specifies the component item to be given focus.  If undefined, clear component focus.
   * @param suppressEvents - Whether to emit focus and blur events
   * @internal
   */
  setFocusedComponentItem(item, suppressEvents = false) {
    if (item !== this._focusedComponentItem) {
      let newFocusedParentItem;
      if (item === void 0) {
        newFocusedParentItem === void 0;
      } else {
        newFocusedParentItem = item.parentItem;
      }
      if (this._focusedComponentItem !== void 0) {
        const oldFocusedItem = this._focusedComponentItem;
        this._focusedComponentItem = void 0;
        oldFocusedItem.setBlurred(suppressEvents);
        const oldFocusedParentItem = oldFocusedItem.parentItem;
        if (newFocusedParentItem === oldFocusedParentItem) {
          newFocusedParentItem = void 0;
        } else {
          oldFocusedParentItem.setFocusedValue(false);
        }
      }
      if (item !== void 0) {
        this._focusedComponentItem = item;
        item.setFocused(suppressEvents);
        if (newFocusedParentItem !== void 0) {
          newFocusedParentItem.setFocusedValue(true);
        }
      }
    }
  }
  /** @internal */
  createContentItemFromConfig(config, parent) {
    switch (config.type) {
      case ItemType.ground:
        throw new AssertError("LMCCIFC68871");
      case ItemType.row:
        return new RowOrColumn(false, this, config, parent);
      case ItemType.column:
        return new RowOrColumn(true, this, config, parent);
      case ItemType.stack:
        return new Stack(this, config, parent);
      case ItemType.component:
        return new ComponentItem(this, config, parent);
      default:
        throw new UnreachableCaseError("CCC913564", config.type, "Invalid Config Item type specified");
    }
  }
  /**
   * This should only be called from stack component.
   * Stack will look after docking processing associated with maximise/minimise
   * @internal
   **/
  setMaximisedStack(stack) {
    if (stack === void 0) {
      if (this._maximisedStack !== void 0) {
        this.processMinimiseMaximisedStack();
      }
    } else {
      if (stack !== this._maximisedStack) {
        if (this._maximisedStack !== void 0) {
          this.processMinimiseMaximisedStack();
        }
        this.processMaximiseStack(stack);
      }
    }
  }
  checkMinimiseMaximisedStack() {
    if (this._maximisedStack !== void 0) {
      this._maximisedStack.minimise();
    }
  }
  // showAllActiveContentItems() was called from ContentItem.show().  Not sure what its purpose was so have commented out
  // Everything seems to work ok without this.  Have left commented code just in case there was a reason for it becomes
  // apparent
  // /** @internal */
  // showAllActiveContentItems(): void {
  //     const allStacks = this.getAllStacks();
  //     for (let i = 0; i < allStacks.length; i++) {
  //         const stack = allStacks[i];
  //         const activeContentItem = stack.getActiveComponentItem();
  //         if (activeContentItem !== undefined) {
  //             if (!(activeContentItem instanceof ComponentItem)) {
  //                 throw new AssertError('LMSAACIS22298');
  //             } else {
  //                 activeContentItem.container.show();
  //             }
  //         }
  //     }
  // }
  // hideAllActiveContentItems() was called from ContentItem.hide().  Not sure what its purpose was so have commented out
  // Everything seems to work ok without this.  Have left commented code just in case there was a reason for it becomes
  // apparent
  // /** @internal */
  // hideAllActiveContentItems(): void {
  //     const allStacks = this.getAllStacks();
  //     for (let i = 0; i < allStacks.length; i++) {
  //         const stack = allStacks[i];
  //         const activeContentItem = stack.getActiveComponentItem();
  //         if (activeContentItem !== undefined) {
  //             if (!(activeContentItem instanceof ComponentItem)) {
  //                 throw new AssertError('LMSAACIH22298');
  //             } else {
  //                 activeContentItem.container.hide();
  //             }
  //         }
  //     }
  // }
  /** @internal */
  cleanupBeforeMaximisedStackDestroyed(event) {
    if (this._maximisedStack !== null && this._maximisedStack === event.target) {
      this._maximisedStack.off("beforeItemDestroyed", this._maximisedStackBeforeDestroyedListener);
      this._maximisedStack = void 0;
    }
  }
  /**
   * This method is used to get around sandboxed iframe restrictions.
   * If 'allow-top-navigation' is not specified in the iframe's 'sandbox' attribute
   * (as is the case with codepens) the parent window is forbidden from calling certain
   * methods on the child, such as window.close() or setting document.location.href.
   *
   * This prevented GoldenLayout popouts from popping in in codepens. The fix is to call
   * _$closeWindow on the child window's gl instance which (after a timeout to disconnect
   * the invoking method from the close call) closes itself.
   *
   * @internal
   */
  closeWindow() {
    globalThis.setTimeout(() => globalThis.close(), 1);
  }
  /** @internal */
  getArea(x, y) {
    let matchingArea = null;
    let smallestSurface = Infinity;
    for (let i = 0; i < this._itemAreas.length; i++) {
      const area = this._itemAreas[i];
      if (x >= area.x1 && x < area.x2 && // x2 is not included in area
      y >= area.y1 && y < area.y2 && // y2 is not included in area
      smallestSurface > area.surface) {
        smallestSurface = area.surface;
        matchingArea = area;
      }
    }
    return matchingArea;
  }
  /** @internal */
  calculateItemAreas() {
    const allContentItems = this.getAllContentItems();
    const groundItem = this._groundItem;
    if (groundItem === void 0) {
      throw new UnexpectedUndefinedError("LMCIAR44365");
    } else {
      if (allContentItems.length === 1) {
        const groundArea = groundItem.getElementArea();
        if (groundArea === null) {
          throw new UnexpectedNullError("LMCIARA44365");
        } else {
          this._itemAreas = [groundArea];
        }
        return;
      } else {
        if (groundItem.contentItems[0].isStack) {
          this._itemAreas = [];
        } else {
          this._itemAreas = groundItem.createSideAreas();
        }
        for (let i = 0; i < allContentItems.length; i++) {
          const stack = allContentItems[i];
          if (ContentItem.isStack(stack)) {
            const area = stack.getArea();
            if (area === null) {
              continue;
            } else {
              this._itemAreas.push(area);
              const stackContentAreaDimensions = stack.contentAreaDimensions;
              if (stackContentAreaDimensions === void 0) {
                throw new UnexpectedUndefinedError("LMCIASC45599");
              } else {
                const highlightArea = stackContentAreaDimensions.header.highlightArea;
                const surface = (highlightArea.x2 - highlightArea.x1) * (highlightArea.y2 - highlightArea.y1);
                const header = {
                  x1: highlightArea.x1,
                  x2: highlightArea.x2,
                  y1: highlightArea.y1,
                  y2: highlightArea.y2,
                  contentItem: stack,
                  surface
                };
                this._itemAreas.push(header);
              }
            }
          }
        }
      }
    }
  }
  /**
   * Called as part of loading a new layout (including initial init()).
   * Checks to see layout has a maximised item. If so, it maximises that item.
   * @internal
   */
  checkLoadedLayoutMaximiseItem() {
    if (this._groundItem === void 0) {
      throw new UnexpectedUndefinedError("LMCLLMI43432");
    } else {
      const configMaximisedItems = this._groundItem.getConfigMaximisedItems();
      if (configMaximisedItems.length > 0) {
        let item = configMaximisedItems[0];
        if (ContentItem.isComponentItem(item)) {
          const stack = item.parent;
          if (stack === null) {
            throw new UnexpectedNullError("LMXLLMI69999");
          } else {
            item = stack;
          }
        }
        if (!ContentItem.isStack(item)) {
          throw new AssertError("LMCLLMI19993");
        } else {
          item.maximise();
        }
      }
    }
  }
  /** @internal */
  processMaximiseStack(stack) {
    this._maximisedStack = stack;
    stack.on("beforeItemDestroyed", this._maximisedStackBeforeDestroyedListener);
    stack.element.classList.add(
      "lm_maximised"
      /* Maximised */
    );
    stack.element.insertAdjacentElement("afterend", this._maximisePlaceholder);
    if (this._groundItem === void 0) {
      throw new UnexpectedUndefinedError("LMMXI19993");
    } else {
      this._groundItem.element.prepend(stack.element);
      const { width, height } = getElementWidthAndHeight(this._containerElement);
      setElementWidth(stack.element, width);
      setElementHeight(stack.element, height);
      stack.updateSize(true);
      stack.focusActiveContentItem();
      this._maximisedStack.emit("maximised");
      this.emit("stateChanged");
    }
  }
  /** @internal */
  processMinimiseMaximisedStack() {
    if (this._maximisedStack === void 0) {
      throw new AssertError("LMMMS74422");
    } else {
      const stack = this._maximisedStack;
      if (stack.parent === null) {
        throw new UnexpectedNullError("LMMI13668");
      } else {
        stack.element.classList.remove(
          "lm_maximised"
          /* Maximised */
        );
        this._maximisePlaceholder.insertAdjacentElement("afterend", stack.element);
        this._maximisePlaceholder.remove();
        this.updateRootSize(true);
        this._maximisedStack = void 0;
        stack.off("beforeItemDestroyed", this._maximisedStackBeforeDestroyedListener);
        stack.emit("minimised");
        this.emit("stateChanged");
      }
    }
  }
  /**
   * Iterates through the array of open popout windows and removes the ones
   * that are effectively closed. This is necessary due to the lack of reliably
   * listening for window.close / unload events in a cross browser compatible fashion.
   * @internal
   */
  reconcilePopoutWindows() {
    const openPopouts = [];
    for (let i = 0; i < this._openPopouts.length; i++) {
      if (this._openPopouts[i].getWindow().closed === false) {
        openPopouts.push(this._openPopouts[i]);
      } else {
        this.emit("windowClosed", this._openPopouts[i]);
      }
    }
    if (this._openPopouts.length !== openPopouts.length) {
      this._openPopouts = openPopouts;
      this.emit("stateChanged");
    }
  }
  /**
   * Returns a flattened array of all content items,
   * regardles of level or type
   * @internal
   */
  getAllContentItems() {
    if (this._groundItem === void 0) {
      throw new UnexpectedUndefinedError("LMGACI13130");
    } else {
      return this._groundItem.getAllContentItems();
    }
  }
  /**
   * Creates Subwindows (if there are any). Throws an error
   * if popouts are blocked.
   * @internal
   */
  createSubWindows() {
    for (let i = 0; i < this.layoutConfig.openPopouts.length; i++) {
      const popoutConfig = this.layoutConfig.openPopouts[i];
      this.createPopoutFromPopoutLayoutConfig(popoutConfig);
    }
  }
  /**
   * Debounces resize events
   * @internal
   */
  handleContainerResize() {
    if (this.resizeWithContainerAutomatically) {
      this.processResizeWithDebounce();
    }
  }
  /**
   * Debounces resize events
   * @internal
   */
  processResizeWithDebounce() {
    if (this.resizeDebounceExtendedWhenPossible) {
      this.checkClearResizeTimeout();
    }
    if (this._resizeTimeoutId === void 0) {
      this._resizeTimeoutId = setTimeout(() => {
        this._resizeTimeoutId = void 0;
        this.beginSizeInvalidation();
        this.endSizeInvalidation();
      }, this.resizeDebounceInterval);
    }
  }
  checkClearResizeTimeout() {
    if (this._resizeTimeoutId !== void 0) {
      clearTimeout(this._resizeTimeoutId);
      this._resizeTimeoutId = void 0;
    }
  }
  /**
   * Determines what element the layout will be created in
   * @internal
   */
  setContainer() {
    var _a;
    const bodyElement = document.body;
    const containerElement = (_a = this._containerElement) !== null && _a !== void 0 ? _a : bodyElement;
    if (containerElement === bodyElement) {
      this.resizeWithContainerAutomatically = true;
      const documentElement = document.documentElement;
      documentElement.style.height = "100%";
      documentElement.style.margin = "0";
      documentElement.style.padding = "0";
      documentElement.style.overflow = "clip";
      bodyElement.style.height = "100%";
      bodyElement.style.margin = "0";
      bodyElement.style.padding = "0";
      bodyElement.style.overflow = "clip";
    }
    this._containerElement = containerElement;
  }
  /**
   * Called when the window is closed or the user navigates away
   * from the page
   * @internal
   * @deprecated to be removed in version 3
   */
  onBeforeUnload() {
    this.destroy();
  }
  /**
   * Adjusts the number of columns to be lower to fit the screen and still maintain minItemWidth.
   * @internal
   */
  adjustColumnsResponsive() {
    if (this._groundItem === void 0) {
      throw new UnexpectedUndefinedError("LMACR20883");
    } else {
      this._firstLoad = false;
      if (this.useResponsiveLayout() && !this._updatingColumnsResponsive && this._groundItem.contentItems.length > 0 && this._groundItem.contentItems[0].isRow) {
        if (this._groundItem === void 0 || this._width === null) {
          throw new UnexpectedUndefinedError("LMACR77412");
        } else {
          const columnCount = this._groundItem.contentItems[0].contentItems.length;
          if (columnCount <= 1) {
            return;
          } else {
            const minItemWidth = this.layoutConfig.dimensions.defaultMinItemWidth;
            const totalMinWidth = columnCount * minItemWidth;
            if (totalMinWidth <= this._width) {
              return;
            } else {
              this._updatingColumnsResponsive = true;
              const finalColumnCount = Math.max(Math.floor(this._width / minItemWidth), 1);
              const stackColumnCount = columnCount - finalColumnCount;
              const rootContentItem = this._groundItem.contentItems[0];
              const allStacks = this.getAllStacks();
              if (allStacks.length === 0) {
                throw new AssertError("LMACRS77413");
              } else {
                const firstStackContainer = allStacks[0];
                for (let i = 0; i < stackColumnCount; i++) {
                  const column = rootContentItem.contentItems[rootContentItem.contentItems.length - 1];
                  this.addChildContentItemsToContainer(firstStackContainer, column);
                }
                this._updatingColumnsResponsive = false;
              }
            }
          }
        }
      }
    }
  }
  /**
   * Determines if responsive layout should be used.
   *
   * @returns True if responsive layout should be used; otherwise false.
   * @internal
   */
  useResponsiveLayout() {
    const settings = this.layoutConfig.settings;
    const alwaysResponsiveMode = settings.responsiveMode === ResponsiveMode.always;
    const onLoadResponsiveModeAndFirst = settings.responsiveMode === ResponsiveMode.onload && this._firstLoad;
    return alwaysResponsiveMode || onLoadResponsiveModeAndFirst;
  }
  /**
   * Adds all children of a node to another container recursively.
   * @param container - Container to add child content items to.
   * @param node - Node to search for content items.
   * @internal
   */
  addChildContentItemsToContainer(container, node) {
    const contentItems = node.contentItems;
    if (node instanceof Stack) {
      for (let i = 0; i < contentItems.length; i++) {
        const item = contentItems[i];
        node.removeChild(item, true);
        container.addChild(item);
      }
    } else {
      for (let i = 0; i < contentItems.length; i++) {
        const item = contentItems[i];
        this.addChildContentItemsToContainer(container, item);
      }
    }
  }
  /**
   * Finds all the stacks.
   * @returns The found stack containers.
   * @internal
   */
  getAllStacks() {
    if (this._groundItem === void 0) {
      throw new UnexpectedUndefinedError("LMFASC52778");
    } else {
      const stacks = [];
      this.findAllStacksRecursive(stacks, this._groundItem);
      return stacks;
    }
  }
  /** @internal */
  findFirstContentItemType(type) {
    if (this._groundItem === void 0) {
      throw new UnexpectedUndefinedError("LMFFCIT82446");
    } else {
      return this.findFirstContentItemTypeRecursive(type, this._groundItem);
    }
  }
  /** @internal */
  findFirstContentItemTypeRecursive(type, node) {
    const contentItems = node.contentItems;
    const contentItemCount = contentItems.length;
    if (contentItemCount === 0) {
      return void 0;
    } else {
      for (let i = 0; i < contentItemCount; i++) {
        const contentItem = contentItems[i];
        if (contentItem.type === type) {
          return contentItem;
        }
      }
      for (let i = 0; i < contentItemCount; i++) {
        const contentItem = contentItems[i];
        const foundContentItem = this.findFirstContentItemTypeRecursive(type, contentItem);
        if (foundContentItem !== void 0) {
          return foundContentItem;
        }
      }
      return void 0;
    }
  }
  /** @internal */
  findFirstContentItemTypeByIdRecursive(type, id, node) {
    const contentItems = node.contentItems;
    const contentItemCount = contentItems.length;
    if (contentItemCount === 0) {
      return void 0;
    } else {
      for (let i = 0; i < contentItemCount; i++) {
        const contentItem = contentItems[i];
        if (contentItem.type === type && contentItem.id === id) {
          return contentItem;
        }
      }
      for (let i = 0; i < contentItemCount; i++) {
        const contentItem = contentItems[i];
        const foundContentItem = this.findFirstContentItemTypeByIdRecursive(type, id, contentItem);
        if (foundContentItem !== void 0) {
          return foundContentItem;
        }
      }
      return void 0;
    }
  }
  /**
   * Finds all the stack containers.
   *
   * @param stacks - Set of containers to populate.
   * @param node - Current node to process.
   * @internal
   */
  findAllStacksRecursive(stacks, node) {
    const contentItems = node.contentItems;
    for (let i = 0; i < contentItems.length; i++) {
      const item = contentItems[i];
      if (item instanceof Stack) {
        stacks.push(item);
      } else {
        if (!item.isComponent) {
          this.findAllStacksRecursive(stacks, item);
        }
      }
    }
  }
  /** @internal */
  findFirstLocation(selectors) {
    const count = selectors.length;
    for (let i = 0; i < count; i++) {
      const selector = selectors[i];
      const location2 = this.findLocation(selector);
      if (location2 !== void 0) {
        return location2;
      }
    }
    return void 0;
  }
  /** @internal */
  findLocation(selector) {
    const selectorIndex = selector.index;
    switch (selector.typeId) {
      case 0: {
        if (this._focusedComponentItem === void 0) {
          return void 0;
        } else {
          const parentItem = this._focusedComponentItem.parentItem;
          const parentContentItems = parentItem.contentItems;
          const parentContentItemCount = parentContentItems.length;
          if (selectorIndex === void 0) {
            return { parentItem, index: parentContentItemCount };
          } else {
            const focusedIndex = parentContentItems.indexOf(this._focusedComponentItem);
            const index = focusedIndex + selectorIndex;
            if (index < 0 || index > parentContentItemCount) {
              return void 0;
            } else {
              return { parentItem, index };
            }
          }
        }
      }
      case 1: {
        if (this._focusedComponentItem === void 0) {
          return void 0;
        } else {
          const parentItem = this._focusedComponentItem.parentItem;
          return this.tryCreateLocationFromParentItem(parentItem, selectorIndex);
        }
      }
      case 2: {
        const parentItem = this.findFirstContentItemType(ItemType.stack);
        if (parentItem === void 0) {
          return void 0;
        } else {
          return this.tryCreateLocationFromParentItem(parentItem, selectorIndex);
        }
      }
      case 3: {
        let parentItem = this.findFirstContentItemType(ItemType.row);
        if (parentItem !== void 0) {
          return this.tryCreateLocationFromParentItem(parentItem, selectorIndex);
        } else {
          parentItem = this.findFirstContentItemType(ItemType.column);
          if (parentItem !== void 0) {
            return this.tryCreateLocationFromParentItem(parentItem, selectorIndex);
          } else {
            return void 0;
          }
        }
      }
      case 4: {
        const parentItem = this.findFirstContentItemType(ItemType.row);
        if (parentItem === void 0) {
          return void 0;
        } else {
          return this.tryCreateLocationFromParentItem(parentItem, selectorIndex);
        }
      }
      case 5: {
        const parentItem = this.findFirstContentItemType(ItemType.column);
        if (parentItem === void 0) {
          return void 0;
        } else {
          return this.tryCreateLocationFromParentItem(parentItem, selectorIndex);
        }
      }
      case 6: {
        if (this._groundItem === void 0) {
          throw new UnexpectedUndefinedError("LMFLRIF18244");
        } else {
          if (this.rootItem !== void 0) {
            return void 0;
          } else {
            if (selectorIndex === void 0 || selectorIndex === 0)
              return { parentItem: this._groundItem, index: 0 };
            else {
              return void 0;
            }
          }
        }
      }
      case 7: {
        if (this._groundItem === void 0) {
          throw new UnexpectedUndefinedError("LMFLF18244");
        } else {
          const groundContentItems = this._groundItem.contentItems;
          if (groundContentItems.length === 0) {
            if (selectorIndex === void 0 || selectorIndex === 0)
              return { parentItem: this._groundItem, index: 0 };
            else {
              return void 0;
            }
          } else {
            const parentItem = groundContentItems[0];
            return this.tryCreateLocationFromParentItem(parentItem, selectorIndex);
          }
        }
      }
    }
  }
  /** @internal */
  tryCreateLocationFromParentItem(parentItem, selectorIndex) {
    const parentContentItems = parentItem.contentItems;
    const parentContentItemCount = parentContentItems.length;
    if (selectorIndex === void 0) {
      return { parentItem, index: parentContentItemCount };
    } else {
      if (selectorIndex < 0 || selectorIndex > parentContentItemCount) {
        return void 0;
      } else {
        return { parentItem, index: selectorIndex };
      }
    }
  }
};
(function(LayoutManager2) {
  function createMaximisePlaceElement(document2) {
    const element = document2.createElement("div");
    element.classList.add(
      "lm_maximise_place"
      /* MaximisePlace */
    );
    return element;
  }
  LayoutManager2.createMaximisePlaceElement = createMaximisePlaceElement;
  function createTabDropPlaceholderElement(document2) {
    const element = document2.createElement("div");
    element.classList.add(
      "lm_drop_tab_placeholder"
      /* DropTabPlaceholder */
    );
    return element;
  }
  LayoutManager2.createTabDropPlaceholderElement = createTabDropPlaceholderElement;
  LayoutManager2.defaultLocationSelectors = [
    { typeId: 1, index: void 0 },
    { typeId: 2, index: void 0 },
    { typeId: 3, index: void 0 },
    { typeId: 7, index: void 0 }
  ];
  LayoutManager2.afterFocusedItemIfPossibleLocationSelectors = [
    { typeId: 0, index: 1 },
    { typeId: 2, index: void 0 },
    { typeId: 3, index: void 0 },
    { typeId: 7, index: void 0 }
  ];
})(LayoutManager || (LayoutManager = {}));

// dist/esm/ts/virtual-layout.js
var VirtualLayout = class _VirtualLayout extends LayoutManager {
  /** @internal */
  constructor(configOrOptionalContainer, containerOrBindComponentEventHandler, unbindComponentEventHandler, skipInit) {
    super(_VirtualLayout.createLayoutManagerConstructorParameters(configOrOptionalContainer, containerOrBindComponentEventHandler));
    this._bindComponentEventHanlderPassedInConstructor = false;
    this._creationTimeoutPassed = false;
    if (containerOrBindComponentEventHandler !== void 0) {
      if (typeof containerOrBindComponentEventHandler === "function") {
        this.bindComponentEvent = containerOrBindComponentEventHandler;
        this._bindComponentEventHanlderPassedInConstructor = true;
        if (unbindComponentEventHandler !== void 0) {
          this.unbindComponentEvent = unbindComponentEventHandler;
        }
      }
    }
    if (!this._bindComponentEventHanlderPassedInConstructor) {
      if (this.isSubWindow) {
        if (this._constructorOrSubWindowLayoutConfig === void 0) {
          throw new UnexpectedUndefinedError("VLC98823");
        } else {
          const resolvedLayoutConfig = LayoutConfig.resolve(this._constructorOrSubWindowLayoutConfig);
          this.layoutConfig = Object.assign(Object.assign({}, resolvedLayoutConfig), { root: void 0 });
        }
      }
    }
    if (skipInit !== true) {
      if (!this.deprecatedConstructor) {
        this.init();
      }
    }
  }
  destroy() {
    this.bindComponentEvent = void 0;
    this.unbindComponentEvent = void 0;
    super.destroy();
  }
  /**
   * Creates the actual layout. Must be called after all initial components
   * are registered. Recurses through the configuration and sets up
   * the item tree.
   *
   * If called before the document is ready it adds itself as a listener
   * to the document.ready event
   * @deprecated LayoutConfig should not be loaded in {@link (LayoutManager:class)} constructor, but rather in a
   * {@link (LayoutManager:class).loadLayout} call.  If LayoutConfig is not specified in {@link (LayoutManager:class)} constructor,
   * then init() will be automatically called internally and should not be called externally.
   */
  init() {
    if (!this._bindComponentEventHanlderPassedInConstructor && (document.readyState === "loading" || document.body === null)) {
      document.addEventListener("DOMContentLoaded", () => this.init(), { passive: true });
      return;
    }
    if (!this._bindComponentEventHanlderPassedInConstructor && this.isSubWindow === true && !this._creationTimeoutPassed) {
      setTimeout(() => this.init(), 7);
      this._creationTimeoutPassed = true;
      return;
    }
    if (this.isSubWindow === true) {
      if (!this._bindComponentEventHanlderPassedInConstructor) {
        this.clearHtmlAndAdjustStylesForSubWindow();
      }
      window.__glInstance = this;
    }
    super.init();
  }
  /**
   * Clears existing HTML and adjusts style to make window suitable to be a popout sub window
   * Curently is automatically called when window is a subWindow and bindComponentEvent is not passed in the constructor
   * If bindComponentEvent is not passed in the constructor, the application must either call this function explicitly or
   * (preferably) make the window suitable as a subwindow.
   * In the future, it is planned that this function is NOT automatically called in any circumstances.  Applications will
   * need to determine whether a window is a Golden Layout popout window and either call this function explicitly or
   * hide HTML not relevant to the popout.
   * See apitest for an example of how HTML is hidden when popout windows are displayed
   */
  clearHtmlAndAdjustStylesForSubWindow() {
    const headElement = document.head;
    const appendNodeLists = new Array(4);
    appendNodeLists[0] = document.querySelectorAll("body link");
    appendNodeLists[1] = document.querySelectorAll("body style");
    appendNodeLists[2] = document.querySelectorAll("template");
    appendNodeLists[3] = document.querySelectorAll(".gl_keep");
    for (let listIdx = 0; listIdx < appendNodeLists.length; listIdx++) {
      const appendNodeList = appendNodeLists[listIdx];
      for (let nodeIdx = 0; nodeIdx < appendNodeList.length; nodeIdx++) {
        const node = appendNodeList[nodeIdx];
        headElement.appendChild(node);
      }
    }
    const bodyElement = document.body;
    bodyElement.innerHTML = "";
    bodyElement.style.visibility = "visible";
    this.checkAddDefaultPopinButton();
    const x = document.body.offsetHeight;
  }
  /**
   * Will add button if not popinOnClose specified in settings
   * @returns true if added otherwise false
   */
  checkAddDefaultPopinButton() {
    if (this.layoutConfig.settings.popInOnClose) {
      return false;
    } else {
      const popInButtonElement = document.createElement("div");
      popInButtonElement.classList.add(
        "lm_popin"
        /* Popin */
      );
      popInButtonElement.setAttribute("title", this.layoutConfig.header.dock);
      const iconElement = document.createElement("div");
      iconElement.classList.add(
        "lm_icon"
        /* Icon */
      );
      const bgElement = document.createElement("div");
      bgElement.classList.add(
        "lm_bg"
        /* Bg */
      );
      popInButtonElement.appendChild(iconElement);
      popInButtonElement.appendChild(bgElement);
      popInButtonElement.addEventListener("click", () => this.emit("popIn"));
      document.body.appendChild(popInButtonElement);
      return true;
    }
  }
  /** @internal */
  bindComponent(container, itemConfig) {
    if (this.bindComponentEvent !== void 0) {
      const bindableComponent = this.bindComponentEvent(container, itemConfig);
      return bindableComponent;
    } else {
      if (this.getComponentEvent !== void 0) {
        return {
          virtual: false,
          component: this.getComponentEvent(container, itemConfig)
        };
      } else {
        const text = i18nStrings[
          2
          /* ComponentTypeNotRegisteredAndBindComponentEventHandlerNotAssigned */
        ];
        const message = `${text}: ${JSON.stringify(itemConfig)}`;
        throw new BindError(message);
      }
    }
  }
  /** @internal */
  unbindComponent(container, virtual, component) {
    if (this.unbindComponentEvent !== void 0) {
      this.unbindComponentEvent(container);
    } else {
      if (!virtual && this.releaseComponentEvent !== void 0) {
        if (component === void 0) {
          throw new UnexpectedUndefinedError("VCUCRCU333998");
        } else {
          this.releaseComponentEvent(container, component);
        }
      }
    }
  }
};
(function(VirtualLayout2) {
  let subWindowChecked = false;
  function createLayoutManagerConstructorParameters(configOrOptionalContainer, containerOrBindComponentEventHandler) {
    const windowConfigKey = subWindowChecked ? null : new URL(document.location.href).searchParams.get("gl-window");
    subWindowChecked = true;
    const isSubWindow = windowConfigKey !== null;
    let containerElement;
    let config;
    if (windowConfigKey !== null) {
      const windowConfigStr = localStorage.getItem(windowConfigKey);
      if (windowConfigStr === null) {
        throw new Error("Null gl-window Config");
      }
      localStorage.removeItem(windowConfigKey);
      const minifiedWindowConfig = JSON.parse(windowConfigStr);
      const resolvedConfig = ResolvedLayoutConfig.unminifyConfig(minifiedWindowConfig);
      config = LayoutConfig.fromResolved(resolvedConfig);
      if (configOrOptionalContainer instanceof HTMLElement) {
        containerElement = configOrOptionalContainer;
      }
    } else {
      if (configOrOptionalContainer === void 0) {
        config = void 0;
      } else {
        if (configOrOptionalContainer instanceof HTMLElement) {
          config = void 0;
          containerElement = configOrOptionalContainer;
        } else {
          config = configOrOptionalContainer;
        }
      }
      if (containerElement === void 0) {
        if (containerOrBindComponentEventHandler instanceof HTMLElement) {
          containerElement = containerOrBindComponentEventHandler;
        }
      }
    }
    return {
      constructorOrSubWindowLayoutConfig: config,
      isSubWindow,
      containerElement
    };
  }
  VirtualLayout2.createLayoutManagerConstructorParameters = createLayoutManagerConstructorParameters;
})(VirtualLayout || (VirtualLayout = {}));

// dist/esm/ts/golden-layout.js
var GoldenLayout = class extends VirtualLayout {
  /** @internal */
  constructor(configOrOptionalContainer, containerOrBindComponentEventHandler, unbindComponentEventHandler) {
    super(configOrOptionalContainer, containerOrBindComponentEventHandler, unbindComponentEventHandler, true);
    this._componentTypesMap = /* @__PURE__ */ new Map();
    this._registeredComponentMap = /* @__PURE__ */ new Map();
    this._virtuableComponentMap = /* @__PURE__ */ new Map();
    this._containerVirtualRectingRequiredEventListener = (container, width, height) => this.handleContainerVirtualRectingRequiredEvent(container, width, height);
    this._containerVirtualVisibilityChangeRequiredEventListener = (container, visible) => this.handleContainerVirtualVisibilityChangeRequiredEvent(container, visible);
    this._containerVirtualZIndexChangeRequiredEventListener = (container, logicalZIndex, defaultZIndex) => this.handleContainerVirtualZIndexChangeRequiredEvent(container, logicalZIndex, defaultZIndex);
    if (!this.deprecatedConstructor) {
      this.init();
    }
  }
  /**
   * Register a new component type with the layout manager.
   *
   * @deprecated See {@link https://stackoverflow.com/questions/40922531/how-to-check-if-a-javascript-function-is-a-constructor}
   * instead use {@link (GoldenLayout:class).registerComponentConstructor}
   * or {@link (GoldenLayout:class).registerComponentFactoryFunction}
   */
  registerComponent(name, componentConstructorOrFactoryFtn, virtual = false) {
    if (typeof componentConstructorOrFactoryFtn !== "function") {
      throw new ApiError("registerComponent() componentConstructorOrFactoryFtn parameter is not a function");
    } else {
      if (componentConstructorOrFactoryFtn.hasOwnProperty("prototype")) {
        const componentConstructor = componentConstructorOrFactoryFtn;
        this.registerComponentConstructor(name, componentConstructor, virtual);
      } else {
        const componentFactoryFtn = componentConstructorOrFactoryFtn;
        this.registerComponentFactoryFunction(name, componentFactoryFtn, virtual);
      }
    }
  }
  /**
   * Register a new component type with the layout manager.
   */
  registerComponentConstructor(typeName, componentConstructor, virtual = false) {
    if (typeof componentConstructor !== "function") {
      throw new Error(i18nStrings[
        1
        /* PleaseRegisterAConstructorFunction */
      ]);
    }
    const existingComponentType = this._componentTypesMap.get(typeName);
    if (existingComponentType !== void 0) {
      throw new BindError(`${i18nStrings[
        3
        /* ComponentIsAlreadyRegistered */
      ]}: ${typeName}`);
    }
    this._componentTypesMap.set(typeName, {
      constructor: componentConstructor,
      factoryFunction: void 0,
      virtual
    });
  }
  /**
   * Register a new component with the layout manager.
   */
  registerComponentFactoryFunction(typeName, componentFactoryFunction, virtual = false) {
    if (typeof componentFactoryFunction !== "function") {
      throw new BindError("Please register a constructor function");
    }
    const existingComponentType = this._componentTypesMap.get(typeName);
    if (existingComponentType !== void 0) {
      throw new BindError(`${i18nStrings[
        3
        /* ComponentIsAlreadyRegistered */
      ]}: ${typeName}`);
    }
    this._componentTypesMap.set(typeName, {
      constructor: void 0,
      factoryFunction: componentFactoryFunction,
      virtual
    });
  }
  /**
   * Register a component function with the layout manager. This function should
   * return a constructor for a component based on a config.
   * This function will be called if a component type with the required name is not already registered.
   * It is recommended that applications use the {@link (VirtualLayout:class).getComponentEvent} and
   * {@link (VirtualLayout:class).releaseComponentEvent} instead of registering a constructor callback
   * @deprecated use {@link (GoldenLayout:class).registerGetComponentConstructorCallback}
   */
  registerComponentFunction(callback) {
    this.registerGetComponentConstructorCallback(callback);
  }
  /**
   * Register a callback closure with the layout manager which supplies a Component Constructor.
   * This callback should return a constructor for a component based on a config.
   * This function will be called if a component type with the required name is not already registered.
   * It is recommended that applications use the {@link (VirtualLayout:class).getComponentEvent} and
   * {@link (VirtualLayout:class).releaseComponentEvent} instead of registering a constructor callback
   */
  registerGetComponentConstructorCallback(callback) {
    if (typeof callback !== "function") {
      throw new Error("Please register a callback function");
    }
    if (this._getComponentConstructorFtn !== void 0) {
      console.warn("Multiple component functions are being registered.  Only the final registered function will be used.");
    }
    this._getComponentConstructorFtn = callback;
  }
  getRegisteredComponentTypeNames() {
    const typeNamesIterableIterator = this._componentTypesMap.keys();
    return Array.from(typeNamesIterableIterator);
  }
  /**
   * Returns a previously registered component instantiator.  Attempts to utilize registered
   * component type by first, then falls back to the component constructor callback function (if registered).
   * If neither gets an instantiator, then returns `undefined`.
   * Note that `undefined` will return if config.componentType is not a string
   *
   * @param config - The item config
   * @public
   */
  getComponentInstantiator(config) {
    let instantiator;
    const typeName = ResolvedComponentItemConfig.resolveComponentTypeName(config);
    if (typeName !== void 0) {
      instantiator = this._componentTypesMap.get(typeName);
    }
    if (instantiator === void 0) {
      if (this._getComponentConstructorFtn !== void 0) {
        instantiator = {
          constructor: this._getComponentConstructorFtn(config),
          factoryFunction: void 0,
          virtual: false
        };
      }
    }
    return instantiator;
  }
  /** @internal */
  bindComponent(container, itemConfig) {
    let instantiator;
    const typeName = ResolvedComponentItemConfig.resolveComponentTypeName(itemConfig);
    if (typeName !== void 0) {
      instantiator = this._componentTypesMap.get(typeName);
    }
    if (instantiator === void 0) {
      if (this._getComponentConstructorFtn !== void 0) {
        instantiator = {
          constructor: this._getComponentConstructorFtn(itemConfig),
          factoryFunction: void 0,
          virtual: false
        };
      }
    }
    let result;
    if (instantiator !== void 0) {
      const virtual = instantiator.virtual;
      let componentState;
      if (itemConfig.componentState === void 0) {
        componentState = void 0;
      } else {
        componentState = deepExtendValue({}, itemConfig.componentState);
      }
      let component;
      const componentConstructor = instantiator.constructor;
      if (componentConstructor !== void 0) {
        component = new componentConstructor(container, componentState, virtual);
      } else {
        const factoryFunction = instantiator.factoryFunction;
        if (factoryFunction !== void 0) {
          component = factoryFunction(container, componentState, virtual);
        } else {
          throw new AssertError("LMBCFFU10008");
        }
      }
      if (virtual) {
        if (component === void 0) {
          throw new UnexpectedUndefinedError("GLBCVCU988774");
        } else {
          const virtuableComponent = component;
          const componentRootElement = virtuableComponent.rootHtmlElement;
          if (componentRootElement === void 0) {
            throw new BindError(`${i18nStrings[
              5
              /* VirtualComponentDoesNotHaveRootHtmlElement */
            ]}: ${typeName}`);
          } else {
            ensureElementPositionAbsolute(componentRootElement);
            this.container.appendChild(componentRootElement);
            this._virtuableComponentMap.set(container, virtuableComponent);
            container.virtualRectingRequiredEvent = this._containerVirtualRectingRequiredEventListener;
            container.virtualVisibilityChangeRequiredEvent = this._containerVirtualVisibilityChangeRequiredEventListener;
            container.virtualZIndexChangeRequiredEvent = this._containerVirtualZIndexChangeRequiredEventListener;
          }
        }
      }
      this._registeredComponentMap.set(container, component);
      result = {
        virtual: instantiator.virtual,
        component
      };
    } else {
      result = super.bindComponent(container, itemConfig);
    }
    return result;
  }
  /** @internal */
  unbindComponent(container, virtual, component) {
    const registeredComponent = this._registeredComponentMap.get(container);
    if (registeredComponent === void 0) {
      super.unbindComponent(container, virtual, component);
    } else {
      const virtuableComponent = this._virtuableComponentMap.get(container);
      if (virtuableComponent !== void 0) {
        const componentRootElement = virtuableComponent.rootHtmlElement;
        if (componentRootElement === void 0) {
          throw new AssertError("GLUC77743", container.title);
        } else {
          this.container.removeChild(componentRootElement);
          this._virtuableComponentMap.delete(container);
        }
      }
    }
  }
  fireBeforeVirtualRectingEvent(count) {
    this._goldenLayoutBoundingClientRect = this.container.getBoundingClientRect();
    super.fireBeforeVirtualRectingEvent(count);
  }
  /** @internal */
  handleContainerVirtualRectingRequiredEvent(container, width, height) {
    const virtuableComponent = this._virtuableComponentMap.get(container);
    if (virtuableComponent === void 0) {
      throw new UnexpectedUndefinedError("GLHCSCE55933");
    } else {
      const rootElement = virtuableComponent.rootHtmlElement;
      if (rootElement === void 0) {
        throw new BindError(i18nStrings[
          4
          /* ComponentIsNotVirtuable */
        ] + " " + container.title);
      } else {
        const containerBoundingClientRect = container.element.getBoundingClientRect();
        const left = containerBoundingClientRect.left - this._goldenLayoutBoundingClientRect.left;
        rootElement.style.left = numberToPixels(left);
        const top = containerBoundingClientRect.top - this._goldenLayoutBoundingClientRect.top;
        rootElement.style.top = numberToPixels(top);
        setElementWidth(rootElement, width);
        setElementHeight(rootElement, height);
      }
    }
  }
  /** @internal */
  handleContainerVirtualVisibilityChangeRequiredEvent(container, visible) {
    const virtuableComponent = this._virtuableComponentMap.get(container);
    if (virtuableComponent === void 0) {
      throw new UnexpectedUndefinedError("GLHCVVCRE55934");
    } else {
      const rootElement = virtuableComponent.rootHtmlElement;
      if (rootElement === void 0) {
        throw new BindError(i18nStrings[
          4
          /* ComponentIsNotVirtuable */
        ] + " " + container.title);
      } else {
        setElementDisplayVisibility(rootElement, visible);
      }
    }
  }
  /** @internal */
  handleContainerVirtualZIndexChangeRequiredEvent(container, logicalZIndex, defaultZIndex) {
    const virtuableComponent = this._virtuableComponentMap.get(container);
    if (virtuableComponent === void 0) {
      throw new UnexpectedUndefinedError("GLHCVZICRE55935");
    } else {
      const rootElement = virtuableComponent.rootHtmlElement;
      if (rootElement === void 0) {
        throw new BindError(i18nStrings[
          4
          /* ComponentIsNotVirtuable */
        ] + " " + container.title);
      } else {
        rootElement.style.zIndex = defaultZIndex;
      }
    }
  }
};
export {
  ApiError,
  BindError,
  BrowserPopout,
  ComponentContainer,
  ComponentItem,
  ComponentItemConfig,
  ConfigurationError,
  ContentItem,
  DragSource,
  EventEmitter,
  EventHub,
  ExternalError,
  GoldenLayout,
  Header,
  HeaderedItemConfig,
  I18nStrings,
  ItemConfig,
  ItemType,
  JsonValue,
  LayoutConfig,
  LayoutManager,
  LogicalZIndex,
  LogicalZIndexToDefaultMap,
  PopoutBlockedError,
  PopoutLayoutConfig,
  ResolvedComponentItemConfig,
  ResolvedGroundItemConfig,
  ResolvedHeaderedItemConfig,
  ResolvedItemConfig,
  ResolvedLayoutConfig,
  ResolvedPopoutLayoutConfig,
  ResolvedRootItemConfig,
  ResolvedRowOrColumnItemConfig,
  ResolvedStackItemConfig,
  ResponsiveMode,
  RootItemConfig,
  RowOrColumn,
  RowOrColumnItemConfig,
  Side,
  SizeUnitEnum,
  Stack,
  StackItemConfig,
  StyleConstants,
  Tab,
  VirtualLayout,
  WidthOrHeightPropertyName,
  formatSize,
  formatUndefinableSize,
  i18nStrings,
  parseSize
};
