#!/usr/bin/env python3
"""Generate every result image + timing table for Assignment 3.

  python tools/run_all.py [--images images] [--results results] [--exe path/to/FeatureLensBatch]

Needs: Python 3 + Pillow (pip install pillow) and the FeatureLensBatch executable
(built by build.bat / build.sh; if it is not found it is compiled with g++ / clang++).
Outputs (in results/):
  harris/ mineig/ sift/ matching/   result images (color + grayscale versions)
  timings.csv, report.md            computation times for every case
"""
import argparse, csv, itertools, os, shutil, subprocess, sys, tempfile
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
EXTS = {".png", ".jpg", ".jpeg", ".bmp", ".tif", ".tiff"}


def find_exe(arg):
    names = ["FeatureLensBatch.exe", "FeatureLensBatch"]
    cands = [Path(arg)] if arg else []
    for d in ["build/Release", "build", "build/Debug", "build-batch", "."]:
        cands += [ROOT / d / n for n in names]
    for c in cands:
        if c.is_file():
            return str(c)
    for cxx in ("g++", "clang++"):
        if shutil.which(cxx):
            out = ROOT / "build-batch" / "FeatureLensBatch"
            out.parent.mkdir(exist_ok=True)
            srcs = [str(p) for p in (ROOT / "src/features").glob("*.cpp")] + \
                   [str(ROOT / "src/common/Image.cpp"), str(ROOT / "src/common/Draw.cpp")]
            print("compiling FeatureLensBatch with", cxx)
            subprocess.check_call([cxx, "-O2", "-std=c++17", "-I", str(ROOT / "src"),
                                   str(ROOT / "tools/batch.cpp"), *srcs, "-o", str(out)])
            return str(out)
    sys.exit("FeatureLensBatch not found - run build.bat / build.sh first.")


def run(exe, *args):
    r = subprocess.run([exe, *map(str, args)], capture_output=True, text=True)
    if r.returncode:
        sys.exit(f"{exe} {' '.join(map(str, args))}\n{r.stderr}")
    return r.stdout.split()


def save_ppm(img, path):
    img.convert("RGB").save(path, format="PPM")


def transformed(img):
    """Rotate 15 deg, scale 0.9, brighten: a realistic second view for matching."""
    w, h = img.size
    img = img.resize((int(w * 0.9), int(h * 0.9)), Image.BICUBIC)
    img = img.rotate(15, resample=Image.BICUBIC, expand=True, fillcolor=(255, 255, 255))
    return Image.eval(img, lambda v: min(255, v + 20))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--images", default=str(ROOT / "images"))
    ap.add_argument("--results", default=str(ROOT / "results"))
    ap.add_argument("--exe", default="")
    a = ap.parse_args()
    exe = find_exe(a.exe)
    res = Path(a.results)
    files = sorted(p for p in Path(a.images).rglob("*") if p.suffix.lower() in EXTS)
    if not files:
        sys.exit(f"no images found in {a.images}")
    for d in ("harris", "mineig", "sift", "matching"):
        (res / d).mkdir(parents=True, exist_ok=True)

    rows = []  # case, image, variant, count/matches, time columns
    tmp = Path(tempfile.mkdtemp())
    for f in files:
        name = f.stem
        img = Image.open(f).convert("RGB")
        src = tmp / f"{name}.ppm"
        save_ppm(img, src)

        for variant in ("color", "gray"):
            extra = ["gray"] if variant == "gray" else []
            for algo in ("harris", "mineig", "sift"):
                out = tmp / "out.ppm"
                v = run(exe, algo, src, out, *extra)
                Image.open(out).save(res / algo / f"{name}_{variant}.png")
                if algo == "sift":
                    rows.append([algo, name, variant, v[0], f"detect {v[1]} ms", f"describe {v[2]} ms"])
                else:
                    rows.append([algo, name, variant, v[0], f"{v[1]} ms", ""])
                print(f"{algo:7s} {name:22s} {variant:5s} -> {v}")

        # matching: original vs a rotated/scaled/brighter copy
        t = transformed(img)
        t.save(res / "matching" / f"{name}_view2.png")
        tsrc = tmp / f"{name}_view2.ppm"
        save_ppm(t, tsrc)
        for metric in ("ssd", "ncc"):
            out = tmp / "m.ppm"
            v = run(exe, "match", src, tsrc, out, metric)
            Image.open(out).save(res / "matching" / f"{name}_{metric}.png")
            rows.append([metric, name, "vs rotated copy", v[2], f"match {v[4]} ms", f"SIFT(A+B) {v[3]} ms"])
            print(f"{metric:7s} {name:22s} kp {v[0]}/{v[1]} matches {v[2]} match {v[4]} ms")

    # matching across the image set (every pair), original images
    (res / "matching" / "pairs").mkdir(exist_ok=True)
    for f1, f2 in itertools.combinations(files, 2):
        s1, s2 = tmp / f"{f1.stem}.ppm", tmp / f"{f2.stem}.ppm"
        for metric in ("ssd", "ncc"):
            out = tmp / "p.ppm"
            v = run(exe, "match", s1, s2, out, metric)
            Image.open(out).save(res / "matching" / "pairs" / f"{f1.stem}__{f2.stem}_{metric}.png")
            rows.append([metric, f"{f1.stem} <-> {f2.stem}", "image pair", v[2], f"match {v[4]} ms", f"SIFT(A+B) {v[3]} ms"])

    with open(res / "timings.csv", "w", newline="") as fh:
        w = csv.writer(fh)
        w.writerow(["algorithm", "image", "variant", "points_or_matches", "time", "extra_time"])
        w.writerows(rows)
    with open(res / "report.md", "w") as fh:
        fh.write("# Assignment 3 results\n\n| Algorithm | Image | Variant | Points / matches | Time | Extra |\n|---|---|---|---|---|---|\n")
        for r in rows:
            fh.write("| " + " | ".join(r) + " |\n")
    shutil.rmtree(tmp, ignore_errors=True)
    print("\nDone ->", res)


if __name__ == "__main__":
    main()
