_comparable = [
    "Point",
    "Rect",
    "Polygon",
    "Line",
    "bool",
    "char *",
    "char",
    "char*",
    "double",
    "float",
    "int",
    "int8_t",
    "long double",
    "long long",
    "long",
    "short",
    "int16_t",
    "std::string",
    "string",
    "uint",
    "unint_32t",
]
std = [
    "bool",
    "char *",
    "char",
    "char*",
    "double",
    "float",
    "int",
    "int8_t",
    "long double",
    "long long",
    "long",
    "short",
    "int16_t",
    "std::string",
    "string",
    "uint",
    "unint_32t",
]

_removable = ["const", "static", "unsigned"]


def _stem(s):
    src = s.split(" ")
    target = []
    for item in src:
        if item not in _removable:
            target.append(item)
    return " ".join([str(elem) for elem in target])


def get_struct(name, structs):
    for struct in structs:
        if struct["name"] == name:
            return struct
    return None


def components(structs, name, _type):
    if _stem(_type) in _comparable or is_ref(_type):
        return [name]
    struct = get_struct(_type.rstrip(" *"), structs)
    if struct is not None:
        ret = []
        for field in struct["fields"]:
            target = components(structs, field["name"], field["type"])
            if _type.find("*") == -1:
                ret.extend([name + "." + str(elem) for elem in target])
            else:
                ret.extend([name + "->" + str(elem) for elem in target])
        return ret
    return []


def add_once_to_dict(src, target):
    if isinstance(src, list):
        for obj in src:
            target.setdefault(obj, [])
    elif src not in target:
        target.setdefault(src, [])
    return target


def is_bit_fields(field, structs):
    if "bits" in field:
        return True
    struct = get_struct(field["type"], structs)
    if struct is None:
        return False
    for struct_field in struct["fields"]:
        if is_bit_fields(struct_field, structs):
            return True
    return False


def get_functional_name(name):
    if name.islower():
        return "".join(
            [n.capitalize() for n in name.replace("tbl", "table").split("_")]
        )
    return name


def get_class_index(schema, name):
    for i in range(len(schema["classes"])):
        if schema["classes"][i]["name"] == name:
            return i
    return -1


def get_table_name(name):
    if len(name) > 2 and name[:2] == "db":
        name = name[2:]
    return f"_{name.lower()}_tbl"


def is_ref(type_name):
    return type_name.startswith("dbId<") and type_name[-1] == ">"


def is_hash_table(type_name):
    return type_name.startswith("dbHashTable<") and type_name[-1] == ">"


def get_hash_table_type(type_name):
    if not is_hash_table(type_name) or len(type_name) < 13:
        return None

    return type_name[12:-1] + "*"


def is_pass_by_ref(type_name):
    return type_name.find("dbVector") == 0 or type_name.find("std::vector") == 0


def is_set_by_ref(type_name):
    return (
        type_name == "std::string"
        or type_name.startswith("std::pair")
        or type_name.find("std::vector") == 0
    )


def _is_template_type(type_name):
    open_bracket = type_name.find("<")
    if open_bracket == -1:
        return False

    closed_bracket = type_name.find(">")

    return closed_bracket >= open_bracket


def get_template_type(type_name):
    if not _is_template_type(type_name):
        return None
    num_brackets = 1

    open_bracket = type_name.find("<")
    for i in range(open_bracket + 1, len(type_name)):
        if type_name[i] == "<":
            num_brackets += 1
        elif type_name[i] == ">":
            num_brackets -= 1
            if num_brackets == 0:
                closed_bracket = i

    return type_name[open_bracket + 1 : closed_bracket]


def get_ref_type(type_name):
    if not is_ref(type_name) or len(type_name) < 7:
        return None

    return type_name[6:-1] + "*"
