#!/usr/bin/env python3
"""
JOCKY Agent - Forensic Telemetry Collection
Cross-platform agent for collecting system telemetry on Windows and Linux.
"""

import sys
import os
import json
import platform
import subprocess
import hashlib
import time
from typing import Dict, List, Any, Optional
from dataclasses import dataclass, asdict
from abc import ABC, abstractmethod


@dataclass
class ProcessInfo:
    """Process information structure"""
    pid: int
    ppid: int
    name: str
    cmdline: str
    user: str
    cpu_percent: float = 0.0
    memory_mb: float = 0.0


@dataclass
class NetworkConnection:
    """Network connection information structure"""
    local_ip: str
    local_port: int
    remote_ip: str
    remote_port: int
    protocol: str
    state: str
    pid: int
    process_name: str


@dataclass
class FileInfo:
    """File information structure"""
    path: str
    size: int
    modified_time: float
    hash_md5: str
    hash_sha256: str
    permissions: str


class TelemetryCollector(ABC):
    """Abstract base class for platform-specific telemetry collection"""

    @abstractmethod
    def enumerate_processes(self) -> List[ProcessInfo]:
        pass

    @abstractmethod
    def enumerate_network_connections(self) -> List[NetworkConnection]:
        pass

    @abstractmethod
    def hash_file(self, path: str) -> Dict[str, str]:
        pass

    @abstractmethod
    def get_file_info(self, path: str) -> FileInfo:
        pass


class WindowsCollector(TelemetryCollector):
    """Windows-specific telemetry collection using WMI and PowerShell"""

    def enumerate_processes(self) -> List[ProcessInfo]:
        """Enumerate processes using WMI"""
        processes = []
        try:
            # Use PowerShell to get process info via WMI
            cmd = [
                "powershell", "-Command",
                "Get-WmiObject Win32_Process | Select-Object ProcessId, ParentProcessId, Name, CommandLine, @{Name='User';Expression={$_.GetOwner().User}} | ConvertTo-Json"
            ]
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)

            if result.returncode == 0 and result.stdout.strip():
                data = json.loads(result.stdout)
                if not isinstance(data, list):
                    data = [data]

                for proc in data:
                    try:
                        processes.append(ProcessInfo(
                            pid=int(proc.get('ProcessId', 0)),
                            ppid=int(proc.get('ParentProcessId', 0)),
                            name=str(proc.get('Name', '')),
                            cmdline=str(proc.get('CommandLine', '')),
                            user=str(proc.get('User', ''))
                        ))
                    except (ValueError, TypeError):
                        continue
        except Exception as e:
            print(f"Error enumerating processes: {e}", file=sys.stderr)

        return processes

    def enumerate_network_connections(self) -> List[NetworkConnection]:
        """Enumerate network connections using netstat and Get-NetTCPConnection"""
        connections = []
        try:
            cmd = [
                "powershell", "-Command",
                "Get-NetTCPConnection -ErrorAction SilentlyContinue | Select-Object LocalAddress, LocalPort, RemoteAddress, RemotePort, State, OwningProcess | ConvertTo-Json"
            ]
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)

            if result.returncode == 0 and result.stdout.strip():
                data = json.loads(result.stdout)
                if not isinstance(data, list):
                    data = [data]

                for conn in data:
                    try:
                        # Get process name for PID
                        pid = int(conn.get('OwningProcess', 0))
                        proc_name = self._get_process_name(pid)

                        connections.append(NetworkConnection(
                            local_ip=str(conn.get('LocalAddress', '')),
                            local_port=int(conn.get('LocalPort', 0)),
                            remote_ip=str(conn.get('RemoteAddress', '')),
                            remote_port=int(conn.get('RemotePort', 0)),
                            protocol='TCP',
                            state=str(conn.get('State', '')),
                            pid=pid,
                            process_name=proc_name
                        ))
                    except (ValueError, TypeError):
                        continue
        except Exception as e:
            print(f"Error enumerating network connections: {e}", file=sys.stderr)

        return connections

    def _get_process_name(self, pid: int) -> str:
        """Get process name by PID"""
        try:
            cmd = ["powershell", "-Command", f"(Get-Process -Id {pid} -ErrorAction SilentlyContinue).ProcessName"]
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=5)
            if result.returncode == 0:
                return result.stdout.strip()
        except Exception:
            pass
        return "unknown"

    def hash_file(self, path: str) -> Dict[str, str]:
        """Calculate file hashes (MD5 and SHA256)"""
        hashes = {"md5": "", "sha256": ""}
        try:
            md5_hash = hashlib.md5()
            sha256_hash = hashlib.sha256()

            with open(path, "rb") as f:
                for chunk in iter(lambda: f.read(8192), b""):
                    md5_hash.update(chunk)
                    sha256_hash.update(chunk)

            hashes["md5"] = md5_hash.hexdigest()
            hashes["sha256"] = sha256_hash.hexdigest()
        except Exception as e:
            print(f"Error hashing file {path}: {e}", file=sys.stderr)

        return hashes

    def get_file_info(self, path: str) -> FileInfo:
        """Get file information"""
        try:
            stat = os.stat(path)
            hashes = self.hash_file(path)

            return FileInfo(
                path=path,
                size=stat.st_size,
                modified_time=stat.st_mtime,
                hash_md5=hashes["md5"],
                hash_sha256=hashes["sha256"],
                permissions=oct(stat.st_mode)[-3:]
            )
        except Exception as e:
            print(f"Error getting file info for {path}: {e}", file=sys.stderr)
            return FileInfo(path=path, size=0, modified_time=0, hash_md5="", hash_sha256="", permissions="")


class LinuxCollector(TelemetryCollector):
    """Linux-specific telemetry collection using /proc filesystem"""

    def enumerate_processes(self) -> List[ProcessInfo]:
        """Enumerate processes using /proc filesystem"""
        processes = []
        try:
            proc_dir = "/proc"
            for entry in os.listdir(proc_dir):
                if entry.isdigit():
                    pid = int(entry)
                    proc_info = self._read_process_info(pid)
                    if proc_info:
                        processes.append(proc_info)
        except Exception as e:
            print(f"Error enumerating processes: {e}", file=sys.stderr)

        return processes

    def _read_process_info(self, pid: int) -> Optional[ProcessInfo]:
        """Read process information from /proc/<pid>"""
        try:
            # Read stat file
            with open(f"/proc/{pid}/stat", "r") as f:
                stat_data = f.read().split()

            # Read cmdline
            cmdline = ""
            try:
                with open(f"/proc/{pid}/cmdline", "r") as f:
                    cmdline = f.read().replace('\x00', ' ')
            except Exception:
                pass

            # Read status for UID
            user = str(stat_data[3]) if len(stat_data) > 3 else "unknown"

            return ProcessInfo(
                pid=pid,
                ppid=int(stat_data[3]) if len(stat_data) > 3 else 0,
                name=stat_data[1].strip('()') if len(stat_data) > 1 else "unknown",
                cmdline=cmdline.strip(),
                user=user
            )
        except Exception:
            return None

    def enumerate_network_connections(self) -> List[NetworkConnection]:
        """Enumerate network connections using /proc/net/tcp and ss command"""
        connections = []
        try:
            # Use ss command for more detailed info
            cmd = ["ss", "-tunap"]
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)

            if result.returncode == 0:
                lines = result.stdout.strip().split('\n')[1:]  # Skip header
                for line in lines:
                    parts = line.split()
                    if len(parts) >= 7:
                        try:
                            proto = parts[0]
                            local_addr = parts[4]
                            remote_addr = parts[5]
                            state = parts[1]

                            # Parse addresses
                            local_ip, local_port = self._parse_address(local_addr)
                            remote_ip, remote_port = self._parse_address(remote_addr)

                            # Parse PID and process name
                            pid = 0
                            proc_name = "unknown"
                            if len(parts) >= 7:
                                proc_info = parts[6]
                                if "pid=" in proc_info:
                                    pid_str = proc_info.split("pid=")[1].split(",")[0]
                                    pid = int(pid_str)
                                    proc_name = proc_info.split("pid=")[1].split(",")[1] if "," in proc_info else "unknown"

                            connections.append(NetworkConnection(
                                local_ip=local_ip,
                                local_port=local_port,
                                remote_ip=remote_ip,
                                remote_port=remote_port,
                                protocol=proto,
                                state=state,
                                pid=pid,
                                process_name=proc_name
                            ))
                        except (ValueError, IndexError):
                            continue
        except Exception as e:
            print(f"Error enumerating network connections: {e}", file=sys.stderr)

        return connections

    def _parse_address(self, addr: str) -> tuple:
        """Parse IP:PORT address"""
        try:
            if ':' in addr:
                parts = addr.rsplit(':', 1)
                return parts[0], int(parts[1])
        except Exception:
            pass
        return "0.0.0.0", 0

    def hash_file(self, path: str) -> Dict[str, str]:
        """Calculate file hashes (MD5 and SHA256)"""
        hashes = {"md5": "", "sha256": ""}
        try:
            md5_hash = hashlib.md5()
            sha256_hash = hashlib.sha256()

            with open(path, "rb") as f:
                for chunk in iter(lambda: f.read(8192), b""):
                    md5_hash.update(chunk)
                    sha256_hash.update(chunk)

            hashes["md5"] = md5_hash.hexdigest()
            hashes["sha256"] = sha256_hash.hexdigest()
        except Exception as e:
            print(f"Error hashing file {path}: {e}", file=sys.stderr)

        return hashes

    def get_file_info(self, path: str) -> FileInfo:
        """Get file information"""
        try:
            stat = os.stat(path)
            hashes = self.hash_file(path)

            return FileInfo(
                path=path,
                size=stat.st_size,
                modified_time=stat.st_mtime,
                hash_md5=hashes["md5"],
                hash_sha256=hashes["sha256"],
                permissions=oct(stat.st_mode)[-3:]
            )
        except Exception as e:
            print(f"Error getting file info for {path}: {e}", file=sys.stderr)
            return FileInfo(path=path, size=0, modified_time=0, hash_md5="", hash_sha256="", permissions="")


class JockyAgent:
    """Main JOCKY Agent class"""

    def __init__(self):
        self.collector = self._get_collector()

    def _get_collector(self) -> TelemetryCollector:
        """Get platform-specific collector"""
        system = platform.system().lower()
        if system == "windows":
            return WindowsCollector()
        elif system == "linux":
            return LinuxCollector()
        else:
            raise RuntimeError(f"Unsupported platform: {system}")

    def run_query(self, query_name: str, params: Dict[str, Any] = None) -> Dict[str, Any]:
        """Run a forensic query"""
        result = {
            "query": query_name,
            "timestamp": time.time(),
            "platform": platform.system(),
            "data": {}
        }

        if query_name == "scan_processes":
            result["data"]["processes"] = [asdict(p) for p in self.collector.enumerate_processes()]
        elif query_name == "scan_network":
            result["data"]["connections"] = [asdict(c) for c in self.collector.enumerate_network_connections()]
        elif query_name == "hash_file":
            if params and "path" in params:
                result["data"]["hashes"] = self.collector.hash_file(params["path"])
        elif query_name == "file_info":
            if params and "path" in params:
                result["data"]["file"] = asdict(self.collector.get_file_info(params["path"]))
        elif query_name == "full_scan":
            result["data"]["processes"] = [asdict(p) for p in self.collector.enumerate_processes()]
            result["data"]["connections"] = [asdict(c) for c in self.collector.enumerate_network_connections()]
        else:
            result["error"] = f"Unknown query: {query_name}"

        return result


def main():
    """Main entry point for JOCKY Agent"""
    if len(sys.argv) < 2:
        print("Usage: jocky-agent <query_name> [params_json]")
        print("Available queries: scan_processes, scan_network, hash_file, file_info, full_scan")
        return 1

    query_name = sys.argv[1]
    params = {}
    if len(sys.argv) > 2:
        try:
            params = json.loads(sys.argv[2])
        except json.JSONDecodeError:
            print("Error: Invalid JSON parameters", file=sys.stderr)
            return 1

    try:
        agent = JockyAgent()
        result = agent.run_query(query_name, params)
        print(json.dumps(result, indent=2))
        return 0
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())