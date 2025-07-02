def toRefString(val):
    return f"(*static_cast<const {val.type}*>({int(val.address)}))"


def typeInfoPtr2TypeName(p_type_info):
    type_info_str = str(p_type_info)
    MATCH_STR = "<typeinfo for "
    # print(str(p_type_info))
    return type_info_str[type_info_str.find(MATCH_STR) + len(MATCH_STR) : -1]


# For Variant class

class Variant_Printer:
    def __init__(self, val):
        self.val = val

    def getContainedPtr(self):
        # print("Variant getContainedPtr!")
        if self.val["m_table"].referenced_value()["hasShortObjectOptimization"] == 1:
            return self.val["m_val"]["m_shortBufferVal"]
        else:
            return self.val["m_val"]["mp_longVal"]

    def getContainedValue(self):
        try:
            # print("Variant getContainedValue!")
            contained_type = gdb.lookup_type(self.getTypeName())
            # print(f"Found type: {contained_type}")
            return (
                self.getContainedPtr()
                .reinterpret_cast(contained_type.pointer())
                .referenced_value()
            )
        except Exception as e:
            print("getContainedValue failed!")
            print(e)
            return

    def getTypeInfoPtr(self):
        # print("Variant getTypeInfo!")
        return (
            self.val["m_table"]
            .referenced_value()["p_typeinfo"]
            .referenced_value()["p_id"]
        )

    def getTypeName(self):
        # print("Variant getTypeName!")
        p_type_info = self.getTypeInfoPtr()
        type_info_str = str(p_type_info)
        MATCH_STR = "<typeinfo for "
        # print(str(p_type_info))
        return typeInfoPtr2TypeName(p_type_info)

    def to_string(self):
        try:
            # print("Variant to_string!")
            return f"Variant: {self.getTypeName()} = {str(self.getContainedValue())}"
        except Exception as e:
            print("Variant failed!")
            print(e)
            return

    def children(self):
        try:
            yield "table", self.val["m_table"]
            yield "contained value", self.getContainedValue()
        except Exception as e:
            print("Variant failed!")
            print(e)
            return


def Variant_Printer_func(val):
    if val.type.name is not None and val.type.unqualified().name.startswith(
        "Vitrae::Variant"
    ):
        return Variant_Printer(val)


gdb.pretty_printers.append(Variant_Printer_func)


# For FirmPtr class
class FirmPtr_Printer:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            p_counter = self.val["m_p_ctr"]
            if int(p_counter) != 0:
                counter = p_counter.referenced_value()
                firm_count = counter["m_firmcount"]
                lazy_count = counter["m_lazycount"]
                return f"0x{int(self.val["m_p_ctr"]):x}[{firm_count}]({lazy_count}) 0x{int(self.val["m_p_object"]):x} <{str(self.val['m_p_object'].referenced_value())}>"
            else:
                return f"<unset> 0x0 0x{int(self.val["m_p_object"]):x}"
        except Exception as e:
            print("FirmPtr failed!")
            print(e)
            return

    def display_hint(self):
        return "array"

    def children(self):
        try:
            p_counter = self.val["m_p_ctr"]
            if int(p_counter) != 0:
                yield "", self.val["m_p_object"].referenced_value()
            else:
                yield "", "<No object>"
        except Exception as e:
            print("FirmPtr failed!")
            print(e)
            return


def FirmPtr_Printer_func(val):
    if val.type.name is not None and val.type.unqualified().name.startswith(
        "dynasma::FirmPtr"
    ):
        return FirmPtr_Printer(val)


gdb.pretty_printers.append(FirmPtr_Printer_func)


# For LazyPtr class
class LazyPtr_Printer:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        try:
            counter = self.val["m_p_ctr"].referenced_value()
            firm_count = counter["m_firmcount"]
            lazy_count = counter["m_lazycount"]
            return f"0x{int(self.val["m_p_ctr"]):x}[{firm_count}]({lazy_count})"
        except Exception as e:
            print("LazyPtr failed!")
            print(e)
            return


def LazyPtr_Printer_func(val):
    if val.type.name is not None and val.type.unqualified().name.startswith(
        "dynasma::LazyPtr"
    ):
        return LazyPtr_Printer(val)


gdb.pretty_printers.append(LazyPtr_Printer_func)

# StringId
class StringId_Printer:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        m_hash = int(self.val["m_hash"])
        if any(field.name == "m_str" for field in self.val.type.fields()):
            m_str = self.val["m_str"].string()
            return f'<#{m_hash:x}> "{m_str}"'
        else:
            return f"<#{m_hash:x}>"


def StringId_Printer_func(val):
    if val.type.name == "Vitrae::StringId":
        return StringId_Printer(val)


gdb.pretty_printers.append(StringId_Printer_func)


# ParamSpec
class ParamSpec_Printer:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        name = self.val["name"]
        typeInfo = self.val["typeInfo"]

        p_typeId = typeInfo["p_id"]

        return f"{name}: {typeInfoPtr2TypeName(p_typeId)}"


def ParamSpec_Printer_func(val):
    if val.type.name == "Vitrae::ParamSpec":
        return ParamSpec_Printer(val)


gdb.pretty_printers.append(ParamSpec_Printer_func)


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
        try:
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
        except Exception as e:
            print("StableMap_Printer failed!")
            print(e)
            return


def StableMap_Printer_func(val):
    if val.type.name is not None and val.type.name.startswith("Vitrae::StableMap"):
        return StableMap_Printer(val)


gdb.pretty_printers.append(StableMap_Printer_func)


class ParamList_Printer:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        specs = self.val["m_specList"]
        name = f"(*(const {specs.type.name}*){specs.address})"
        count = int(gdb.parse_and_eval(f"{name}.size()"))
        return f"{count} Specs"

    def children(self):
        try:
            specs = self.val["m_specList"]
            valRefName = toRefString(specs)
            count = int(gdb.parse_and_eval(f"{valRefName}.size()"))

            for i in range(count):
                yield (str(i), gdb.parse_and_eval(f"{valRefName}[{i}]"))
        except Exception as e:
            print("ParamList_Printer failed!")
            print(e)
            return


def ParamList_Printer_func(val):
    if val.type.name is not None and val.type.unqualified().name.startswith(
        "Vitrae::ParamList"
    ):
        return ParamList_Printer(val)


gdb.pretty_printers.append(ParamList_Printer_func)


# GLM classes


# GlmVec
class GlmVec_Printer:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        v = []
        try:
            v.append(str(self.val["x"]))
        except:
            pass
        try:
            v.append(str(self.val["y"]))
        except:
            pass
        try:
            v.append(str(self.val["z"]))
        except:
            pass
        try:
            v.append(str(self.val["w"]))
        except:
            pass

        return f"[{','.join(v)}]"

    def children(self):
        try:
            try:
                yield "x/r/s", self.val["x"]
            except:
                pass

            try:
                yield "y/g/t", self.val["y"]
            except:
                pass

            try:
                yield "z/b/p", self.val["z"]
            except:
                pass

            try:
                yield "w/a/q", self.val["w"]
            except:
                pass

        except Exception as e:
            print("GlmVec_Printer failed!")
            print(e)
            return


def GlmVec_Printer_func(val):
    if val.type.name is not None and val.type.unqualified().name.startswith("glm::vec"):
        return GlmVec_Printer(val)

gdb.pretty_printers.append(GlmVec_Printer_func)
