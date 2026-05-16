#!/usr/bin/env python3
"""Build signed default.avdb for the Windows service (RBPO binary format)."""

from __future__ import annotations

import argparse
import struct
import zlib
from pathlib import Path

from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding
from cryptography.hazmat.primitives.serialization import pkcs12


def fnv1a64(data: bytes) -> int:
    hash_value = 1469598103934665603
    prime = 1099511628211
    for byte in data:
        hash_value ^= byte
        hash_value = (hash_value * prime) & 0xFFFFFFFFFFFFFFFF
    return hash_value


def to_le_u64(value: int) -> bytes:
    return struct.pack("<Q", value)


def build_record(prefix: int, object_type: int, offset_begin: int, offset_end: int, tail: bytes) -> dict:
    object_signature = to_le_u64(fnv1a64(tail))
    object_signature_length = 8 + len(tail)
    canonical = b"".join(
        [
            struct.pack("<Q", prefix),
            struct.pack("<I", object_signature_length),
            object_signature,
            struct.pack("<q", offset_begin),
            struct.pack("<q", offset_end),
            struct.pack("<B", object_type),
        ]
    )
    return {
        "prefix": prefix,
        "object_signature_length": object_signature_length,
        "object_signature": object_signature,
        "offset_begin": offset_begin,
        "offset_end": offset_end,
        "object_type": object_type,
        "canonical": canonical,
    }


def build_default_records() -> list[dict]:
    pe_prefix = 0x0000000000005A4D
    py_prefix = 0x2020202020202321
    pe_tail = b"PE\x00\x00MAL1"
    py_tail = b"python\x00\x01"
    return [
        build_record(pe_prefix, 1, 0, 4096, pe_tail),
        build_record(py_prefix, 2, 0, 4096, py_tail),
    ]


def sign_bytes(private_key, data: bytes) -> bytes:
    return private_key.sign(data, padding.PKCS1v15(), hashes.SHA256())


def write_avdb(path: Path, private_key, release_date: int) -> None:
    records = build_default_records()
    payload = struct.pack("<Q", release_date)
    payload += struct.pack("<I", len(records))
    for record in records:
        signature = sign_bytes(private_key, record["canonical"])
        payload += struct.pack("<Q", record["prefix"])
        payload += struct.pack("<I", record["object_signature_length"])
        payload += struct.pack("<I", len(record["object_signature"]))
        payload += record["object_signature"]
        payload += struct.pack("<q", record["offset_begin"])
        payload += struct.pack("<q", record["offset_end"])
        payload += struct.pack("<B", record["object_type"])
        payload += struct.pack("<I", len(signature))
        payload += signature

    manifest_signature = sign_bytes(private_key, payload)
    crc = zlib.crc32(payload) & 0xFFFFFFFF

    blob = bytearray()
    blob.extend(b"RBPO")
    blob.extend(struct.pack("<I", 1))
    blob.extend(struct.pack("<I", crc))
    blob.extend(struct.pack("<Q", len(payload)))
    blob.extend(payload)
    blob.extend(struct.pack("<I", len(manifest_signature)))
    blob.extend(manifest_signature)

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(blob)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--pfx", required=True, type=Path)
    parser.add_argument("--password", required=True)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    pfx_data = args.pfx.read_bytes()
    private_key, _, _ = pkcs12.load_key_and_certificates(pfx_data, args.password.encode("utf-8"))
    if private_key is None:
        raise SystemExit("private key not found in pfx")

    release_date = 133801632000000000
    write_avdb(args.output, private_key, release_date)
    print(f"Wrote {args.output} ({args.output.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
