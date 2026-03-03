def check_required_keys(keys: list[str], obj: dict) -> None:
    for key in keys:
        check_required_key(key, obj)

def check_required_key(key: str, obj: dict) -> None:
    if key not in obj:
        raise KeyError(f'Key `{key}` not present in {obj}')
