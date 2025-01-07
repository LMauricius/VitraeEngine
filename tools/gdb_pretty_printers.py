def toRefName(val):
    return f"(*(const {val.type.name}*){int(val.address)})"


# StringId


class StringId_Printer:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        m_hash = int(self.val["m_hash"])
        if any(field.name == "m_str" for field in self.val.type.fields()):
            m_str = self.val["m_str"].string()
            return f'<{m_hash:x}> "{m_str}"'
        else:
            return f"<{m_hash:x}>"


def StringId_Printer_func(val):
    if val.type.name == "Vitrae::StringId":
        return StringId_Printer(val)


gdb.pretty_printers.append(StringId_Printer_func)


# PropertySpec
class PropertySpec_Printer:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        name = self.val["name"]
        typeInfo = self.val["typeInfo"]

        # typeNameStr = gdb.parse_and_eval(
        #     f"{toRefName(typeInfo.referenced_value())}.getShortTypeName()"
        # )
        typeId = typeInfo["p_id"].referenced_value()

        return f"{name}: {typeId}"


def PropertySpec_Printer_func(val):
    if val.type.name == "Vitrae::PropertySpec":
        return PropertySpec_Printer(val)


gdb.pretty_printers.append(PropertySpec_Printer_func)


# StableMap
class StableMap_Printer:
    def __init__(self, val):
        self.val = val
        self.keyT = val.type.template_argument(0)
        self.mappedT = val.type.template_argument(1)

    def to_string(self):
        count = self.val["m_size"]
        return f"{count}-sized StableMap"

    def children(self):
        count = int(self.val["m_size"])
        data = self.val["m_data"]
        keys = data.reinterpret_cast(self.keyT.pointer())
        offset = (
            (count * self.keyT.sizeof)
            if self.mappedT.sizeof < self.keyT.sizeof
            else (
                (count * self.keyT.sizeof + self.mappedT.alignof - 1)
                // self.mappedT.alignof
                * self.mappedT.alignof
            )
        )
        values = (data[offset].address).reinterpret_cast(self.mappedT.pointer())
        for i in range(count):
            yield (str(keys[i]), values[i])


def StableMap_Printer_func(val):
    if val.type.name is not None and val.type.name.startswith("Vitrae::StableMap"):
        return StableMap_Printer(val)


gdb.pretty_printers.append(StableMap_Printer_func)


class PropertyList_Printer:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        specs = self.val["m_specList"]
        name = f"(*(const {specs.type.name}*){specs.address})"
        count = int(gdb.parse_and_eval(f"{name}.size()"))
        return f"{count} Specs"

    def children(self):
        specs = self.val["m_specList"]
        valRefName = toRefName(specs)
        count = int(gdb.parse_and_eval(f"{valRefName}.size()"))

        for i in range(count):
            yield (str(i), gdb.parse_and_eval(f"{valRefName}[{i}]"))


def PropertyList_Printer_func(val):
    if val.type.name is not None and val.type.name.startswith("Vitrae::PropertyList"):
        return PropertyList_Printer(val)
        # subval = val["m_specList"]
        # for ppf in gdb.current_progspace().pretty_printers:
        #    pp = ppf(subval)
        #    if pp is not None:
        #        return pp


gdb.pretty_printers.append(PropertyList_Printer_func)
