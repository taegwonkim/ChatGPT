#!/usr/bin/env python3
"""Small threaded TCP ACK server for the STM32/ESP-AT example."""

import argparse
import socketserver
from datetime import datetime, timezone


class Handler(socketserver.BaseRequestHandler):
    def handle(self) -> None:
        peer = f"{self.client_address[0]}:{self.client_address[1]}"
        print(f"[{datetime.now(timezone.utc).isoformat()}] connected: {peer}")
        try:
            while data := self.request.recv(4096):
                print(f"[{peer}] {data!r}")
                self.request.sendall(b"ACK:" + data)
        except (ConnectionError, TimeoutError) as exc:
            print(f"[{peer}] connection error: {exc}")
        finally:
            print(f"[{datetime.now(timezone.utc).isoformat()}] disconnected: {peer}")


class Server(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True


def main() -> None:
    parser = argparse.ArgumentParser(description="ESP-AT example TCP ACK server")
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=5000)
    args = parser.parse_args()
    with Server((args.host, args.port), Handler) as server:
        print(f"listening on {args.host}:{args.port} (Ctrl-C to stop)")
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            print("\nstopped")


if __name__ == "__main__":
    main()
