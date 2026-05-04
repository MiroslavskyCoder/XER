
ImportModule("FileSystem");
ImportModule("Container");
ImportModule("Util")

class _Class {
    constructor(name) {
        this._name = name;

        this._functions = new Map();
        this._constructors = [];
        this._isPublic = false;
        this._isPrivate = false;
        this._isProtected = false;
        this._isAbstract = false;
        this._isFinal = false;
    }

    setPublic() {
        this._isPublic = true;
    }
    setPrivate() {
        this._isPrivate = true;
    }
    setProtected() {
        this._isProtected = true;
    }

    setAbstract() {
        this._isAbstract = true;
    }

    setFinal() {
        this._isFinal = true;
    }

    setImplementation(class_name) {
        this._implementation = class_name;
    }

    addFunction(function_name, function_code, type_publish = "public", options = {}) {
        this._functions.set(function_name, {
            code: function_code,
            type_publish: type_publish,
            parameters: options.parameters || "",
            returnType: options.returnType || "void",
            isVirtual: options.isVirtual === true,
            isConst: options.isConst === true,
            isOverride: options.isOverride === true,
            isDefault: options.isDefault === true,
            isDelete: options.isDelete === true
        });
    }

    addConstructor(parameters, constructor_code, type_publish = "public", options = {}) {
        this._constructors.push({
            parameters: parameters || "",
            code: constructor_code,
            type_publish: type_publish,
            initializerList: options.initializerList || [],
            isDefault: options.isDefault === true,
            isDelete: options.isDelete === true
        });
    }
}

class _Constructor {
    constructor() { 
        this._includes = new Set();
        this._options = new Map();
        this._classes = new Map();
    }

    addInclude(path) {
        this._includes.add(path);
    }

    addParamtersHeader(option, parameters) {
        const normalizedParameters = Array.isArray(parameters) ? parameters : [parameters];
        if (option == "pragma" && normalizedParameters.length > 0) {
            if (normalizedParameters[0] == "once") {
                this._options.set("pragma_once", true);
            } else if (normalizedParameters[0] == "warning(push)") {
                this._options.set("pragma_warning_push", true);
            } else if (normalizedParameters[0] == "warning(pop)") {
                this._options.set("pragma_warning_pop", true);
            }
        } 
    }

    addParametersHeader(option, parameters) {
        this.addParamtersHeader(option, parameters);
    }

    _renderAccessBlock(result, label, lines) {
        if (lines.length === 0) {
            return result;
        }

        result += `${label}:\n`;
        for (const line of lines) {
            const splitLines = line.split("\n");
            for (const splitLine of splitLines) {
                result += `    ${splitLine}\n`;
            }
        }
        return result;
    }

    _normalizeList(value) {
        if (Array.isArray(value)) {
            return value;
        }
        if (!value) {
            return [];
        }
        return [value];
    }

    _indentCode(code, level = 1) {
        const indent = "    ".repeat(level);
        return String(code)
            .split("\n")
            .map((line) => `${indent}${line}`)
            .join("\n");
    }

    _renderConstructor(class_name, constructorInfo) {
        const parameters = constructorInfo.parameters || "";
        const initializerList = this._normalizeList(constructorInfo.initializerList);
        const defaultOrDeleteSuffix = constructorInfo.isDefault
            ? " = default;"
            : constructorInfo.isDelete
            ? " = delete;"
            : "";

        if (defaultOrDeleteSuffix) {
            return `${class_name}(${parameters})${defaultOrDeleteSuffix}`;
        }

        let signature = `${class_name}(${parameters})`;
        if (initializerList.length > 0) {
            signature += ` : ${initializerList.join(", ")}`;
        }

        if (constructorInfo.code && String(constructorInfo.code).trim().length > 0) {
            return `${signature} {\n${this._indentCode(constructorInfo.code, 1)}\n}`;
        }

        return `${signature} {}`;
    }

    _renderMethod(function_name, function_info) {
        const virtualPrefix = function_info.isVirtual ? "virtual " : "";
        const returnType = function_info.returnType || "void";
        const parameters = function_info.parameters || "";
        const constSuffix = function_info.isConst ? " const" : "";
        const overrideSuffix = function_info.isOverride ? " override" : "";
        const baseSignature = `${virtualPrefix}${returnType} ${function_name}(${parameters})${constSuffix}${overrideSuffix}`;

        if (function_info.isDefault) {
            return `${baseSignature} = default;`;
        }

        if (function_info.isDelete) {
            return `${baseSignature} = delete;`;
        }

        if (function_info.code && String(function_info.code).trim().length > 0) {
            return `${baseSignature} {\n${this._indentCode(function_info.code, 1)}\n}`;
        }

        return `${baseSignature} {}`;
    }

    addClass(class_) {
        if (class_ instanceof _Class) {
            this._classes.set(class_._name, class_);
        }
    }

    toString() {
        let result = "";
        for (const include of this._includes) {
            result += `#include ${include}\n`;
        }
        if (this._options.get("pragma_once")) {
            result += `#pragma once\n`;
        }
        if (this._options.get("pragma_warning_push")) {
            result += `#pragma warning(push)\n`;
        }
        if (this._options.get("pragma_warning_pop")) {
            result += `#pragma warning(pop)\n`;
        }
        for (const [class_name, class_] of this._classes) {
            const publicLines = [];
            const privateLines = [];
            const protectedLines = [];

            for (const constructorInfo of class_._constructors) {
                const body = this._renderConstructor(class_name, constructorInfo);

                if (constructorInfo.type_publish === "private") {
                    privateLines.push(body);
                } else if (constructorInfo.type_publish === "protected") {
                    protectedLines.push(body);
                } else {
                    publicLines.push(body);
                }
            }

            for (const [function_name, function_info] of class_._functions) {
                const body = this._renderMethod(function_name, function_info);

                if (function_info.type_publish === "private") {
                    privateLines.push(body);
                } else if (function_info.type_publish === "protected") {
                    protectedLines.push(body);
                } else {
                    publicLines.push(body);
                }
            }

            result += `class ${class_name} {\n`;
            result = this._renderAccessBlock(result, "public", publicLines);
            result = this._renderAccessBlock(result, "protected", protectedLines);
            result = this._renderAccessBlock(result, "private", privateLines);
            result += `};\n`;
        }
        return result;
    }
}

const c = new _Constructor;

c.addParamtersHeader("pragma", "once");
c.addInclude("<iostream>");

const myClass = new _Class("MyClass");
myClass.setPublic();
myClass.addConstructor("int value", "std::cout << \"MyClass ctor\" << std::endl;", "public", {
    initializerList: ["value_(value)"]
});
myClass.addFunction("myFunction", "std::cout << \"Hello, World!\" << std::endl;", "public", {
    parameters: "int count, const std::string& label",
    returnType: "void",
    isConst: true
});
myClass.addFunction("clone", "", "public", {
    returnType: "MyClass*",
    isVirtual: true,
    isDelete: true
});

c.addClass(myClass);

console.log(FileSystem.writeText("output.cpp", c.toString()));