#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""排查复现驱动：create -> 等待 open 就绪 -> insert_html_content -> save -> close
依据 html-to-doc SKILL §2.1 工具级调用契约（脚本失败的排查路径）。"""
import json
import os
import subprocess
import sys
import time

EDSDK = "/root/.codebuddy/plugins/cache/workbuddy-docx/workbuddy-docx/--WORKBUDDY_BUILD_VERSION-/skills/html-to-doc/scripts/edsdk.py"
HTML = sys.argv[1]
OUTPUT = sys.argv[2]


def call(tool, args=None, timeout=180):
    with open("/tmp/edsdk_args.json", "w", encoding="utf-8") as f:
        json.dump(args or {}, f, ensure_ascii=False)
    p = subprocess.run(
        ["python3", EDSDK, "call", tool, "--json-file", "/tmp/edsdk_args.json"],
        capture_output=True, text=True, timeout=timeout)
    out = p.stdout.strip()
    # edsdk 输出可能带说明行, 取最后一个 JSON 对象
    try:
        return json.loads(out)
    except Exception:
        for line in reversed(out.splitlines()):
            line = line.strip()
            if line.startswith("{"):
                try:
                    return json.loads(line)
                except Exception:
                    continue
        return {"_raw": out}


def extract_kv(text):
    m = {}
    try:
        d = json.loads(text)
        if isinstance(d, dict):
            for k in ("file_id", "file_path"):
                if k in d:
                    m[k] = d[k]
    except Exception:
        pass
    import re
    for k in ("file_id", "file_path"):
        if k not in m:
            r = re.search(k + r"=(\S+?)(?=\s|$|,)", text)
            if r:
                m[k] = r.group(1)
    return m


# 1) create
r = call("create_doc", {"file_path": OUTPUT})
raw = r.get("_raw", "") if isinstance(r, dict) else ""
kv = {k: r[k] for k in ("file_id", "file_path") if isinstance(r, dict) and k in r}
if "file_id" not in kv:
    kv = extract_kv(json.dumps(r) if not raw else raw)
if "file_id" not in kv and raw:
    kv = extract_kv(raw)
fid_create = kv.get("file_id")
created_path = kv.get("file_path")
print("create:", fid_create, created_path)
if not fid_create:
    print("FAIL create", r)
    sys.exit(1)

# 2) open + 轮询就绪
call("open_file", {"file_path": created_path, "wait": True})
live_id = None
for i in range(30):
    time.sleep(1)
    pool = call("get_pool_status", {})
    editors = pool.get("open_editors", []) if isinstance(pool, dict) else []
    for e in editors:
        if e.get("file_path") == created_path:
            live_id = e.get("file_id")
            break
    if live_id:
        break
if not live_id:
    live_id = created_path
print("live_id:", live_id)

# 3) insert 整份 HTML
html_text = open(HTML, encoding="utf-8").read()
r = call("doc_insert_html_content", {"file_id": live_id, "idx": 0, "html_text": html_text}, timeout=600)
print("insert:", json.dumps(r, ensure_ascii=False)[:300])
if not (isinstance(r, dict) and ("last_edit_index" in r or r.get("message", "").find("ok") >= 0 or "last_edit_index" in json.dumps(r))):
    if isinstance(r, dict) and r.get("_raw", "").find("ok") < 0 and "last_edit_index" not in json.dumps(r):
        print("FAIL insert", json.dumps(r, ensure_ascii=False)[:500])
        call("close_file", {"file_id": live_id})
        sys.exit(1)

# 4) save 到交付路径
r = call("save_file", {"file_id": live_id, "file_path": OUTPUT}, timeout=300)
print("save:", json.dumps(r, ensure_ascii=False)[:200])

# 5) close 释放会话
call("close_file", {"file_id": live_id})
print("done:", OUTPUT, os.path.getsize(OUTPUT) if os.path.exists(OUTPUT) else "MISSING")