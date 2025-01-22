#!/usr/bin/env python3
from http.server import BaseHTTPRequestHandler, HTTPServer
import json
import sys
import time

class MockHandler(BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        return  # Silence logs

    def do_POST(self):
        content_length = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(content_length).decode('utf-8')
        payload = json.loads(body)
        
        response_data = {"response": f"Sent Message: {payload.get('query', '')}"}
        if payload.get("list_sources", False):
            response_data["sources"] = ["source_1", "source_2", "source_3"]
            
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(json.dumps(response_data).encode('utf-8'))

def run_server():
    port = 8080
    server = HTTPServer(("0.0.0.0", port), MockHandler)
    print(f"SERVER_READY:{port}", flush=True)
    server.serve_forever()
    sys.exit(1)

if __name__ == "__main__":
    run_server()