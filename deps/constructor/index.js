
ImportModule("FileSystem");
ImportModule("Container");
ImportModule("Util")

class _Class {
    constructor(name) {
        this._name = name;

        this._functions = new Map();
        this._classes = new Map();
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

    addFunction(function_name, function_code, type_publish) {
        this._functions.set(function_name, {
            code: function_code,
            type_publish: type_publish
        });
    }
}

class _Constructor {
    constructor() { 
        this._includes = new Set();
        this._options = new Map();
    }

    addInclude(path) {
        this._includes.add(path);
    }

    addParamtersHeader(option, parameters) {
        if (option == "pragma" && parameters.length > 0) {
            if (parameters[0] == "once") {
                this._options.set("pragma_once", true);
            } else if (parameters[0] == "warning(push)") {
                this._options.set("pragma_warning_push", true);
            } else if (parameters[0] == "warning(pop)") {
                this._options.set("pragma_warning_pop", true);
            }
        } 
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
            result += `class ${class_name} {\n`;
            for (const [function_name, function_info] of class_._functions) {
                result += `    ${function_info.type_publish} ${function_name}() {\n`;
                result += `        ${function_info.code}\n`;
                result += `    }\n`;
            }
            result += `}\n`;
        }
        return result;
    }
}

const c = new _Constructor;

c.addInclude("<iostream>");
c.addParamtersHeader("pragma", "once");

const myClass = new _Class("MyClass");
myClass.setPublic();
myClass.addFunction("myFunction", "std::cout << \"Hello, World!\" << std::endl;", "public");

c.addClass(myClass);

console.log(c.toString());