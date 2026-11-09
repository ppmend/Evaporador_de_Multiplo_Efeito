#!/usr/bin/env python3
"""
Servidor HTTP local simples para execução da interface web e carregamento do WebAssembly.
"""

import http.server
import socketserver
import os
import sys

PORT = 8080
WEB_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "web")

class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=WEB_DIR, **kwargs)

    def end_headers(self):
        # Permite SharedArrayBuffer e carregamento WebAssembly sem bloqueio de isolamento
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Access-Control-Allow-Origin", "*")
        super().end_headers()

def main():
    os.chdir(WEB_DIR)
    with socketserver.TCPServer(("", PORT), Handler) as httpd:
        print(f"================================================================================")
        print(f"  Servidor Web do Simulador de Evaporadores rodando em: http://localhost:{PORT}")
        print(f"  Diretório raiz: {WEB_DIR}")
        print(f"  Pressione Ctrl+C para encerrar o servidor.")
        print(f"================================================================================")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nServidor encerrado.")

if __name__ == "__main__":
    main()

