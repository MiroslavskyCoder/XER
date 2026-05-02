// Функция t(type, msg) вызывается из C++ прежде чем сообщение
// попадёт в консоль. Возвращаемое значение заменяет текст вывода.
function t(type, msg) {
  // Публикуем событие в EventBus чтобы другие модули могли реагировать
  EventBus.publish("console:" + type, msg);
  return "[" + type.toUpperCase() + "]: " + msg;
}

// Подписчик — сработает на каждый console.log
// subscribe принимает (topic, handlerModuleName) — оба строки
EventBus.subscribe("console:log", "onConsoleLog");

function onConsoleLog(msg) {
    return "LOGGED: " + msg;
}

console.log("hello world");
// => [LOG]: hello world

console.warn("что-то пошло не так");
// => [WARN]: что-то пошло не так

console.error("критическая ошибка");
// => [ERROR]: критическая ошибка
