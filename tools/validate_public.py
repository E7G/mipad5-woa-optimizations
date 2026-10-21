"""Validate public file allowlist, provenance and obvious accidental private content."""
from pathlib import Path
import hashlib,json,re
root=Path(__file__).resolve().parents[1]
allowed={'.md','.txt','.py','.ps1','.c','.h','.rc','.tmh','.inf','.sln','.vcxproj','.filters','.patch','.json','.yml'}
special={'.gitignore','.gitattributes','LICENSE'}
provenance=json.loads((root/'evidence/source-provenance.json').read_text(encoding='utf-8'))
for item in provenance['Files']:
    file=root/'drivers/nt36xxx'/item['Path']
    assert hashlib.sha256(file.read_bytes()).hexdigest()==item['PublishedSHA256'],file
count=0
for file in root.rglob('*'):
    rel=file.relative_to(root)
    if '.git' in rel.parts or '__pycache__' in rel.parts or not file.is_file():continue
    assert file.suffix in allowed or file.name in special,rel
    assert file.stat().st_size<1_000_000,rel
    text=file.read_text(encoding='utf-8-sig');assert '\0' not in text,rel
    patterns=[r'-----BEGIN [A-Z ]*PRIVATE KEY-----',r'gh[pousr]_[A-Za-z0-9]{30,}',
              r'S-1-5-21-\d+-\d+-\d+-\d+',r'192\.168\.\d+\.\d+',
              r'[A-Za-z]:[/\\]Users[/\\][A-Za-z]',r'[A-Za-z]:[/\\]CodexBackups[/\\]']
    for pattern in patterns:assert not re.search(pattern,text),str(rel)+' matched private-content pattern'
    count+=1
assert not list((root/'drivers/nt36xxx').glob('Directory.Build.props'))
print('PASS:',count,'public text files; source hashes match; no obvious private content or binaries')
