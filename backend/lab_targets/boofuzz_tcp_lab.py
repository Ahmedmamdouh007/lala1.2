#!/usr/bin/env python3
"""
Boofuzz script for tcp_lab_server (CTF Task 4).
Sends fuzz payloads to localhost:9999. Run tcp_lab_server under ASan first.

  pip install boofuzz
  ./tcp_lab_server   # in one terminal (built with -fsanitize=address)
  python backend/lab_targets/boofuzz_tcp_lab.py   # in another
"""

from boofuzz import Session, Target, Connection
from boofuzz.tests import s_get, s_initialize, s_string

def main():
    target = Target(Connection(host="127.0.0.1", port=9999))
    session = Session(target=target)

    s_initialize("tcp_lab")
    # Fuzzable payload; starting length 24 so early iterations overflow server's 16-byte buffer
    s_string("A" * 24, name="payload")

    session.connect(s_get("tcp_lab"))
    session.fuzz()

if __name__ == "__main__":
    main()
