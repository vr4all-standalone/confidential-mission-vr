"""Run real CM label transforms; independently prove anchors lie on bundled squeeze geometry."""
from pathlib import Path
import json, math, os, struct, subprocess, sys

ROOT = Path(sys.argv[1]); OUT = Path(sys.argv[2]); OUT.mkdir(parents=True, exist_ok=True)
def block(source, signature):
    start = source.index(signature); begin = source.index('{', start); end = begin + 1; depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}'); end += 1
    return source[start:end]

source = (ROOT/'core/rend/vr/xr_controllers.cpp').read_text(encoding='utf-8-sig')
(OUT/'controller-label-production.inc').write_text('\n'.join(block(source, sig) for sig in (
    'glm::vec3 anchor(', 'LabelPoint labelPoint(', 'std::string ownSidecar(')), encoding='utf8')
labels = (ROOT/'core/rend/vr/xr_labels.cpp').read_text(encoding='utf-8-sig')
start = labels.index('const char *name = own::ButtonNames[list[i].button];')
end = labels.index('why.c_str());',start) + len('why.c_str());')
(OUT/'controller-label-diagnostic.inc').write_text(labels[start:end],encoding='utf8')
own = (ROOT/'core/rend/vr/xr_own.h').read_text(encoding='utf-8-sig')
(OUT/'controller-label-names.inc').write_text(block(own,'constexpr const char *ButtonNames[]')+';',encoding='utf8')
env = dict(os.environ); env['PATH'] = 'D:/msys64/mingw64/bin;' + env.get('PATH', '')
binary = OUT/'controller-grip-label-test.exe'
cmd = ['D:/msys64/mingw64/bin/g++.exe','-std=c++17','-O2','-I'+str(OUT),
       '-I'+str(ROOT/'core/rend/vr'),'-I'+str(ROOT/'core/deps/glm'),
       '-I'+str(ROOT/'build-win/_deps/openxr_sdk-src/include'),str(Path(__file__).with_suffix('.cpp')),'-o',str(binary)]
subprocess.run(cmd, env=env, check=True)
points = []
for hand in ('left','right'):
    doc = json.loads((ROOT/f'cm-vr/models/controllers/{hand}.json').read_text(encoding='utf-8-sig'))
    point = doc.get('buttons',{}).get('grip')
    if point is not None: points += [str(p) for p in point]
status = subprocess.run([str(binary), *points], env=env).returncode
if status: sys.exit(status)

proof = []
for hand in ('left','right'):
    data = (ROOT/f'cm-vr/models/controllers/{hand}.glb').read_bytes()
    size, kind = struct.unpack_from('<II', data, 12); doc = json.loads(data[20:20+size]); binary_start = size+28
    def accessor(index):
        a = doc['accessors'][index]; v = doc['bufferViews'][a['bufferView']]
        width = {'VEC3':3,'VEC2':2,'SCALAR':1}[a['type']]; fmt = {5126:'f',5123:'H',5125:'I'}[a['componentType']]
        stride = v.get('byteStride',width*struct.calcsize(fmt)); offset = binary_start+v.get('byteOffset',0)+a.get('byteOffset',0)
        return [struct.unpack_from('<'+fmt*width,data,offset+i*stride) for i in range(a['count'])]
    positions = accessor(0); normals = accessor(1); indices = [i[0] for i in accessor(3)]
    # Vertex2149 is the middle-facing surface of the isolated squeeze button,
    # identified from this CM GLB's geometry, not another mod's coordinates.
    raw = positions[2149]; normal = normals[2149]
    assert abs(normal[0]) > .99 and abs(normal[1]) < .01 and abs(normal[2]) < .04
    parents = list(range(len(positions)))
    def find(i):
        while parents[i] != i: parents[i] = parents[parents[i]]; i = parents[i]
        return i
    def join(a,b): parents[find(a)] = find(b)
    seen = {}
    for i,p in enumerate(positions):
        key = tuple(round(c,6) for c in p)
        if key in seen: join(i,seen[key])
        else: seen[key] = i
    for i in range(0,len(indices),3): join(indices[i],indices[i+1]); join(indices[i],indices[i+2])
    component = [p for i,p in enumerate(positions) if find(i) == find(2149)]
    assert len(component) == 141, 'anchor must stay on the separate squeeze-button geometry'
    lo = [min(p[k] for p in component) for k in range(3)]; hi = [max(p[k] for p in component) for k in range(3)]
    node = doc['nodes'][0]; qx,qy,qz,qw = node['rotation']
    assert abs(qx-math.sqrt(.5)) < 1e-6 and qy == qz == 0 and abs(qw-math.sqrt(.5)) < 1e-6
    assert 'translation' not in node and 'scale' not in node and 'matrix' not in node
    # glb_mesh.cpp applies this +90X node before labelPoint's modelToAim.
    baked = (raw[0],-raw[2],raw[1])
    sidecar = json.loads((ROOT/f'cm-vr/models/controllers/{hand}.json').read_text(encoding='utf-8-sig'))
    point = sidecar['buttons']['grip']; error = math.dist(point,baked)
    assert error < 1e-8, f'{hand} point must be on the actual baked squeeze surface: {error}'
    assert sidecar['scale'] == 1 and sidecar['aim'] == {'position':[0,0,0],'rotation':[0,1,0,0]}
    proof.append(dict(hand=hand,vertex=2149,component_vertices=len(component),raw_vertex=raw,raw_normal=normal,
                      raw_component_bounds=[lo,hi],node_rotation=node['rotation'],baked_vertex=baked,
                      sidecar_grip=point,surface_error_m=error))
(OUT/'coordinate-proof.json').write_text(json.dumps(proof,indent=2)+'\n',encoding='utf8')
print('2 bundled squeeze geometry proofs passed')
