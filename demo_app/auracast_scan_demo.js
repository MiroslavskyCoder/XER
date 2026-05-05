ImportModule("Device");

// Найти все Auracast-источники рядом (колонки, телефоны, авто)
const sources = Device.aura.discover(5000);

console.log("Найдено:", sources.length);

sources.forEach(function(s, i) {
	// Определить тип устройства по имени
	var name = (s.name || "").toLowerCase();
	var type = "Auracast";
	if (/car|auto|bmw|ford|honda|авто/.test(name))          type = "Автомобиль";
	else if (/phone|pixel|samsung|iphone|телефон/.test(name)) type = "Телефон";
	else if (/speaker|sony|jbl|bose|колонк/.test(name))       type = "Колонка";

	var bid = "0x" + (s.broadcastId >>> 0).toString(16).toUpperCase().padStart(6, "0");

	console.log("\n[" + (i + 1) + "] " + type + " — " + (s.name || "без имени"));
	console.log("    addr   :", s.addr);
	console.log("    rssi   :", s.rssi + " dBm");
	console.log("    id     :", bid);
	console.log("    потоки :", s.numBis, s.encrypted ? "(зашифрован)" : "(открытый)");
	console.log("    кодек  :", s.codec || "LC3");

	if (s.bisInfo) {
		s.bisInfo.forEach(function(b) {
			console.log("    BIS[" + b.bisIndex + "]  " +
				b.channelCount + "ch  " +
				b.samplingFreqHz / 1000 + " kHz  " +
				b.octetsPerFrame + " oct/frame");
		});
	}
});

// Подключиться к источнику с лучшим сигналом
if (sources.length > 0) {
	var best = sources.reduce(function(a, b) { return b.rssi > a.rssi ? b : a; });
	console.log("\nЛучший сигнал:", best.name || best.addr, "(" + best.rssi + " dBm)");

	// Синхронизироваться и слушать через C++ API:
	//   AuraCastSession::Instance().SyncWithAudio(source, callback);
	//   AuraCastSession::Instance().GetAudioSink()->SetVolume(0.9f);
}
/**
 * demo_app/auracast_scan_demo.js
 *
 * Пример сканирования Bluetooth LE Audio Auracast-источников:
 *   - Беспроводные колонки
 *   - Телефоны, транслирующие звук
 *   - Автомобильные аудиосистемы
 *
 * Требования:
 *   - BLE 5.2+ адаптер (hci0), Linux ≥ 5.15 (BTPROTO_ISO)
 *   - Права CAP_NET_RAW (или root) для открытия HCI-сокета
 *
 * Запуск:
 *   xer demo_app/auracast_scan_demo.js
 */

ImportModule("Device");

// ─── утилиты ─────────────────────────────────────────────────────────────────

function rssiBar(rssi) {
	const level = Math.max(0, Math.min(5, Math.round((rssi + 100) / 20)));
	return "▓".repeat(level) + "░".repeat(5 - level) + " " + rssi + " dBm";
}

function codecLabel(source) {
	const codec = source.codec || "LC3";
	const hz    = source.bisInfo && source.bisInfo.length > 0
		? (source.bisInfo[0].samplingFreqHz / 1000) + " kHz"
		: "?";
	const dur   = source.bisInfo && source.bisInfo.length > 0
		? (source.bisInfo[0].frameDurationUs / 1000) + " ms"
		: "?";
	return codec + "  " + hz + "  " + dur + "/frame";
}

function deviceCategory(name) {
	const n = (name || "").toLowerCase();
	if (/car|auto|ford|bmw|vw|honda|kia|hyundai|авто|машин/.test(n)) return "Автомобиль";
	if (/phone|pixel|samsung|iphone|xiaomi|huawei|телефон/.test(n))  return "Телефон";
	if (/speaker|колонк|sony|jbl|bose|harman|soundbar/.test(n))      return "Колонка";
	if (/tv|телевизор|display/.test(n))                               return "Телевизор";
	if (/hearing|слухов|aid/.test(n))                                 return "Слуховой аппарат";
	return "Auracast-источник";
}

// ─── 1. Инициализация Device-менеджера ───────────────────────────────────────

console.log("=== Auracast scan demo ===");

Device.initialize();

const mem0 = Device.memStats();
console.log("память (до скана) :", mem0.usedMb + "/" + mem0.totalMb + " MB");

// ─── 2. Сканирование ─────────────────────────────────────────────────────────

const SCAN_MS = 5000;
console.log("\nСканирование " + SCAN_MS / 1000 + " с (BLE HCI passive scan)...");

const sources = Device.aura.discover(SCAN_MS);

console.log("найдено источников:", sources.length);

// ─── 3. Вывод результатов ────────────────────────────────────────────────────

if (sources.length === 0) {
	console.log("[!] Источники не найдены.");
	console.log("    Убедитесь: hciconfig hci0 up  &&  Auracast-устройство рядом");
} else {
	sources.forEach(function(s, i) {
		const cat  = deviceCategory(s.name);
		const lock = s.encrypted ? "enc" : "open";
		const bid  = "0x" + (s.broadcastId >>> 0).toString(16).toUpperCase().padStart(6, "0");

		console.log("\n[" + (i + 1) + "] " + cat + "  —  " + (s.name || "<без имени>"));
		console.log("     addr         :", s.addr);
		console.log("     rssi         :", rssiBar(s.rssi));
		console.log("     broadcast_id :", bid);
		console.log("     bis потоков  :", s.numBis, "  (" + lock + ")");
		console.log("     кодек        :", codecLabel(s));
		console.log("     задержка     :", (s.presentationDelayUs || 0) + " мкс");

		if (s.bisInfo && s.bisInfo.length > 0) {
			s.bisInfo.forEach(function(b) {
				console.log("     BIS[" + b.bisIndex + "]        :",
					b.channelCount + "ch  " +
					b.samplingFreqHz / 1000 + " kHz  " +
					b.octetsPerFrame + " oct/frame");
			});
		}
	});

	// ─── 4. Лучший источник по RSSI ──────────────────────────────────────

	const best = sources.reduce(function(a, b) { return b.rssi > a.rssi ? b : a; });

	console.log("\n─── лучший сигнал ───────────────────────────────────────────");
	console.log("  устройство  :", best.name || best.addr, "  (" + best.rssi + " dBm)");
	console.log("  broadcast_id:", "0x" + (best.broadcastId >>> 0).toString(16).toUpperCase().padStart(6, "0"));
	console.log("  bis потоков :", best.numBis);

	// ─── 5. Как подключиться из C++ ──────────────────────────────────────

	console.log("\n─── подключение к источнику (C++ API) ───────────────────────");
	console.log("  // 1. Получить дескриптор из JS (addr, broadcastId, bisInfo)");
	console.log("  // 2. Вызвать из нативного кода:");
	console.log("  //    auto& session = AuraCastSession::Instance();");
	console.log("  //    session.SyncWithAudio(source, [](AuraCastState s, auto& e){");
	console.log("  //        if (s == AuraCastState::kStreaming) { /* звук идёт */ }");
	console.log("  //    });");
	console.log("  //    session.GetAudioSink()->SetVolume(0.8f);");
	console.log("  //    auto stats = session.GetAudioSink()->GetStats();");
}

// ─── 6. Итоговая статистика ───────────────────────────────────────────────────

const mem1 = Device.memStats();
console.log("\n─── память (после скана) ────────────────────────────────────");
console.log("  " + mem1.usedMb + "/" + mem1.totalMb + " MB  (" + mem1.usagePct.toFixed(1) + "%)");
/**
 * auracast_scan_demo.js
 *
 * Демонстрация сканирования Bluetooth LE Audio Auracast-источников:
 *   - Беспроводные колонки (Bluetooth speaker)
 *   - Телефоны, транслирующие звук (Phone broadcast)
 *   - Автомобильные аудиосистемы (Car infotainment)
 *
 * Требования:
 *   - Bluetooth адаптер с поддержкой BLE 5.2+ (hci0)
 *   - Linux ядро ≥ 5.15 (BTPROTO_ISO)
 *   - Права доступа к /dev/bluetooth (CAP_NET_RAW или запуск от root)
 *
 * Запуск:
 *   xer demo_app/auracast_scan_demo.js
 */

ImportModule("Device");

// ──────────────────────────────────────────────────────────────────────────────
// Утилиты
// ──────────────────────────────────────────────────────────────────────────────

function rssiBar(rssi) {
    // rssi: -100 .. 0 dBm → 0..5 блоков
    const level = Math.max(0, Math.min(5, Math.round((rssi + 100) / 20)));
    return "▓".repeat(level) + "░".repeat(5 - level) + " " + rssi + " dBm";
}

function bisDetails(source) {
    if (!source.bisInfo || source.bisInfo.length === 0) {
        return "  └─ BIS: нет данных";
    }
    return source.bisInfo.map(function(b, i) {
        return "  └─ BIS[" + i + "]  " +
               b.samplingFreqHz / 1000 + " kHz  " +
               b.frameDurationUs / 1000 + " ms  " +
               b.octetsPerFrame + " oct/frame  ch:" + b.channelCount;
    }).join("\n");
}

function deviceCategory(source) {
    const name = (source.name || "").toLowerCase();
    if (/car|auto|ford|bmw|vw|honda|kia|hyundai|avto|авто|машин/.test(name))
        return "🚗 Автомобиль";
    if (/phone|pixel|samsung|iphone|xiaomi|huawei|телефон|смарт/.test(name))
        return "📱 Телефон";
    if (/speaker|колонк|sony|jbl|bose|harman|soundbar|audio/.test(name))
        return "🔊 Колонка";
    if (/tv|телевизор|display/.test(name))
        return "📺 Телевизор";
    if (/hearing|слухов|aid/.test(name))
        return "🦻 Слуховой аппарат";
    return "🎵 Источник Auracast";
}

// ──────────────────────────────────────────────────────────────────────────────
// Сканирование
// ──────────────────────────────────────────────────────────────────────────────

const SCAN_TIMEOUT_MS = 5000;

console.log("╔══════════════════════════════════════════════════════╗");
console.log("║      XER  — Сканирование Bluetooth Auracast          ║");
console.log("║  (BLE 5.2, LE Audio, Public Broadcast Profile 1.0)   ║");
console.log("╚══════════════════════════════════════════════════════╝");
console.log("");
console.log("Сканирование " + SCAN_TIMEOUT_MS / 1000 + " секунд...\n");

const sources = Device.aura.discover(SCAN_TIMEOUT_MS);

if (!sources || sources.length === 0) {
    console.log("Источники Auracast не найдены.");
    console.log("Убедитесь, что:");
    console.log("  1. Bluetooth адаптер включён  (hciconfig hci0 up)");
    console.log("  2. Рядом есть Auracast-совместимые устройства");
    console.log("  3. Запуск выполнен с правами CAP_NET_RAW / root");
} else {
    console.log("Найдено источников: " + sources.length);
    console.log("─".repeat(56));

    sources.forEach(function(source, index) {
        const category = deviceCategory(source);
        const encrypted = source.encrypted ? "🔒 зашифрован" : "🔓 открытый";
        const broadcastIdHex = "0x" + (source.broadcastId >>> 0).toString(16).toUpperCase().padStart(6, "0");

        console.log("\n[" + (index + 1) + "] " + category + "  —  " +
                    (source.name || "<без имени>"));
        console.log("     BD_ADDR     : " + source.addr);
        console.log("     Сигнал      : " + rssiBar(source.rssi));
        console.log("     Broadcast ID: " + broadcastIdHex);
        console.log("     BIS потоков : " + source.numBis);
        console.log("     Шифрование  : " + encrypted);
        if (source.bisInfo) {
            console.log(bisDetails(source));
        }
    });

    console.log("\n" + "─".repeat(56));

    // ──────────────────────────────────────────────────────────────────────
    // Пример: подключиться к первому источнику
    // ──────────────────────────────────────────────────────────────────────
    const best = sources.reduce(function(prev, cur) {
        return cur.rssi > prev.rssi ? cur : prev;
    });

    console.log("\nЛучший сигнал: " + (best.name || best.addr) +
                " (" + best.rssi + " dBm)");
    console.log("Для подключения и прослушивания (native API):");
    console.log("");
    console.log("  // C++:");
    console.log("  AuraCastSession::Instance().SyncWithAudio(source, stateCallback);");
    console.log("  // или из JS через будущий Device.aura.sync(index)");
}

// ──────────────────────────────────────────────────────────────────────────────
// Вывод итоговой статистики памяти / производительности
// ──────────────────────────────────────────────────────────────────────────────
const mem = Device.memStats();
console.log("\n— Память: " +
    mem.usedMb + " / " + mem.totalMb + " MB  (" +
    mem.usagePct.toFixed(1) + "%)");
