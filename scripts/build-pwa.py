"""Refresh the service worker's content-hashed, atomic offline shell cache."""
from pathlib import Path
import hashlib,json
root=Path(__file__).resolve().parents[1]
dist=root/'dist'
assets=sorted(p.relative_to(dist).as_posix() for p in dist.rglob('*') if p.is_file() and p.name!='sw.js')
h=hashlib.sha256()
for a in assets:h.update(a.encode());h.update((dist/a).read_bytes())
version=h.hexdigest()[:16]
template=(root/'scripts'/'sw-template.js').read_text()
(dist/'sw.js').write_text(template.replace('__CACHE_VERSION__',version).replace('__ASSET_LIST__',json.dumps(assets)))
print(f'PWA offline shell {version}: {len(assets)} assets')
