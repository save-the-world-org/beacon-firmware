import CoreBluetooth
import Foundation

// Lists the beacons this Mac hears for N seconds (default 15): Qorvo's Nearby Interaction firmware (QANI), by the
// service it advertises (QNIS) or by its name, "DWM3001CDK (<address>)" from Qorvo's build or "Kelsey Finder" from ours.
// The id is the one CoreBluetooth gives that board on this Mac; a phone gives it another. tools/blescan runs it.
final class Scanner: NSObject, CBCentralManagerDelegate {
    private var central: CBCentralManager!
    private let qnis = CBUUID(string: "2E938FD0-6A61-11ED-A1EB-0242AC120002")
    private(set) var heard = 0
    private(set) var beacons: Set<UUID> = []

    override init() {
        super.init()
        central = CBCentralManager(delegate: self, queue: .main)
    }

    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        switch central.state {
        case .poweredOn:
            central.scanForPeripherals(withServices: nil, options: [CBCentralManagerScanOptionAllowDuplicatesKey: true])
        case .unauthorized:
            print("Bluetooth access refused: allow it for this terminal app in System Settings, Privacy & Security, Bluetooth")
        default:
            print("Bluetooth is not on (state \(central.state.rawValue))")
        }
    }

    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral,
                        advertisementData: [String: Any], rssi: NSNumber) {
        heard += 1
        let name = advertisementData[CBAdvertisementDataLocalNameKey] as? String ?? peripheral.name ?? ""
        let services = advertisementData[CBAdvertisementDataServiceUUIDsKey] as? [CBUUID] ?? []
        let advertisesQNIS = services.contains(qnis)
        guard advertisesQNIS || name.contains("DWM3001") || name.contains("Kelsey Finder") else { return }
        beacons.insert(peripheral.identifier)
        let time = Date().formatted(date: .omitted, time: .standard)
        print(time, "name:", name.isEmpty ? "(not yet)" : name, "rssi:", rssi, "dBm",
              advertisesQNIS ? "QNIS" : "", "id:", peripheral.identifier.uuidString)
    }
}

let seconds = Double(CommandLine.arguments.dropFirst().first ?? "15") ?? 15
let scanner = Scanner()
RunLoop.main.run(until: Date().addingTimeInterval(seconds))
print("advertisements heard: \(scanner.heard), beacons among them: \(scanner.beacons.count)")
