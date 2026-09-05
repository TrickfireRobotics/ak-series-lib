import litgen
import os
import re
import tempfile


# litgen's srcML-based parser mishandles `friend class X;` declarations: when one
# appears anywhere in a class body, the `= delete` (or `= default`) specifier on
# a *later* special member function in that same class gets silently dropped
# during parsing. Concretely, this caused MitModeMotor/ServoModeMotor's
# `MitModeMotor() = delete;` / `ServoModeMotor() = delete;` to be seen as
# ordinary default constructors, so litgen kept emitting `.def(py::init<>())`
# for them even though the default constructor is deleted, which fails to
# compile. Motor/AKSeriesInterface don't hit this because they have no friend
# declarations. Feeding litgen a friend-stripped copy of the header (the real
# header used for compilation is untouched) avoids the bug entirely.
_FRIEND_DECL_REGEX = re.compile(r"^[ \t]*friend[ \t]+(?:class|struct)[ \t]+\w+[ \t]*;[ \t]*\n", re.MULTILINE)


def _litgen_parseable_copy(header_path: str, scratch_dir: str) -> str:
    with open(header_path) as f:
        text = f.read()
    stripped = _FRIEND_DECL_REGEX.sub("", text)
    dest = os.path.join(scratch_dir, os.path.basename(header_path))
    with open(dest, "w") as f:
        f.write(stripped)
    return dest


if "LITGEN_USE_NANOBIND" in os.environ and os.environ["LITGEN_USE_NANOBIND"] == "ON":
    LITGEN_USE_NANOBIND = True
else:
    LITGEN_USE_NANOBIND = False


def my_litgen_options() -> litgen.LitgenOptions:
    # configure your options here
    options = litgen.LitgenOptions()

    if LITGEN_USE_NANOBIND:
        options.bind_library = litgen.BindLibraryType.nanobind

    options.namespaces_root = ["AKSeries"]

    options.srcmlcpp_options.header_filter_acceptable__regex += "|^__[A-Z0-9_]+$"

    options.fn_exclude_by_name__regex = "^priv_"

    options.fn_params_exclude_names__regex = "^priv_"

    options.fn_template_options.add_specialization(
        "^MaxValue$", ["int", "float"], add_suffix_to_function_name=True
    )

    options.fn_template_options.add_specialization(
        "^MinValue$", ["int", "float"], add_suffix_to_function_name=False
    )

    options.fn_return_force_policy_reference_for_references__regex = "Singleton$"

    options.fn_params_replace_modifiable_immutable_by_boxed__regex = "^SwitchBoolValue$"
    # litgen matches fn_exclude_by_name_and_signature against the parameter type exactly
    # as it's spelled at the declaration site, which is unqualified here (the ctors are
    # declared inside `namespace AKSeries { class Motor { ... Motor(Motor &&) ... } }`),
    # not "AKSeries::Motor &&". The fully-qualified form silently never matched, so
    # pybind11 kept getting a `py::init<T&&>()` binding for these move-only classes --
    # which pybind11 cannot compile (no type_caster provides an rvalue-qualified cast_op
    # for the class-under-construction itself). Factory functions that return these types
    # by value (e.g. AKSeriesInterface::createMitMotor) move-construct into the Python
    # object automatically without needing this binding.
    options.fn_exclude_by_name_and_signature["Motor"] = "Motor &&"
    options.fn_exclude_by_name_and_signature["AKSeriesInterface"] = "AKSeriesInterface &&"

    options.fn_vectorize__regex = r".*"

    options.python_run_black_formatter = True

    return options


def autogenerate() -> None:
    repository_dir = os.path.realpath(os.path.dirname(__file__))
    repository_dir = repository_dir.rstrip("python")
    include_dir = repository_dir + "src/"
    header_files = [repository_dir + "include/AKSeries.hpp"]
    print(repository_dir)
    print(include_dir)
    print(header_files)

    output_cpp_pydef_file = (
        repository_dir + "/_pydef_nanobind/nanobind_DaftLib.cpp"
        if LITGEN_USE_NANOBIND
        else repository_dir + "_pydef_pybind11/pybind_DaftLib.cpp"
    )

    with tempfile.TemporaryDirectory() as scratch_dir:
        litgen_header_files = [_litgen_parseable_copy(h, scratch_dir) for h in header_files]

        litgen.write_generated_code_for_files(
            options=my_litgen_options(),
            input_cpp_header_files=litgen_header_files,
            output_cpp_pydef_file=repository_dir + "python/AKSeriesOut.cpp",
            output_stub_pyi_file=repository_dir + "_stubs/ak_series/__init__.pyi",
        )


if __name__ == "__main__":
    autogenerate()
