from __future__ import annotations

import argparse
from pathlib import Path

from tensorflow.lite.python import schema_py_generated as schema_fb


OP_METHODS = {
    "ABS": "AddAbs",
    "ADD": "AddAdd",
    "AVERAGE_POOL_2D": "AddAveragePool2D",
    "CONCATENATION": "AddConcatenation",
    "CONV_2D": "AddConv2D",
    "DEPTHWISE_CONV_2D": "AddDepthwiseConv2D",
    "DEQUANTIZE": "AddDequantize",
    "FULLY_CONNECTED": "AddFullyConnected",
    "LOGISTIC": "AddLogistic",
    "MAX_POOL_2D": "AddMaxPool2D",
    "MEAN": "AddMean",
    "MUL": "AddMul",
    "PAD": "AddPad",
    "PADV2": "AddPadV2",
    "QUANTIZE": "AddQuantize",
    "RELU": "AddRelu",
    "RELU6": "AddRelu6",
    "RESHAPE": "AddReshape",
    "SOFTMAX": "AddSoftmax",
    "SUB": "AddSub",
}


def builtin_operator_names() -> dict[int, str]:
    return {
        value: name
        for name, value in vars(
            schema_fb.BuiltinOperator
        ).items()
        if (
            isinstance(value, int) and
            not name.startswith("_")
        )
    }


def read_operators(model_path: Path) -> list[str]:
    data = model_path.read_bytes()

    model = schema_fb.Model.GetRootAsModel(
        data,
        0
    )

    names_by_code = builtin_operator_names()
    operators: list[str] = []

    for index in range(
        model.OperatorCodesLength()
    ):
        operator_code = model.OperatorCodes(
            index
        )

        code = operator_code.BuiltinCode()

        # Old schemas may store the real value in
        # DeprecatedBuiltinCode.
        if (
            code ==
            schema_fb.BuiltinOperator.PLACEHOLDER_FOR_GREATER_OP_CODES
        ):
            code = operator_code.DeprecatedBuiltinCode()

        name = names_by_code.get(
            code,
            f"UNKNOWN_{code}"
        )

        if (
            name ==
            "CUSTOM"
        ):
            custom = operator_code.CustomCode()

            if custom:
                name = (
                    "CUSTOM:" +
                    custom.decode(
                        "utf-8",
                        errors="replace"
                    )
                )

        if name not in operators:
            operators.append(name)

    return operators


def generate_header(
    operators: list[str]
) -> str:
    unsupported = [
        name
        for name in operators
        if name not in OP_METHODS
    ]

    if unsupported:
        raise RuntimeError(
            "Unsupported/unmapped operators: " +
            ", ".join(unsupported)
        )

    lines = [
        "#pragma once",
        "",
        '#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>',
        "",
        "namespace ModelOps",
        "{",
        f"    constexpr int OPERATOR_COUNT = "
        f"{len(operators)};",
        "",
        "    using Resolver =",
        "        tflite::MicroMutableOpResolver<"
        "OPERATOR_COUNT>;",
        "",
        "    inline Resolver createResolver()",
        "    {",
        "        Resolver resolver;",
        "",
    ]

    for name in operators:
        lines.append(
            f"        resolver."
            f"{OP_METHODS[name]}();"
        )

    lines += [
        "",
        "        return resolver;",
        "    }",
        "}",
        "",
    ]

    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(
        description=(
            "Generate the exact TensorFlow Lite Micro "
            "operator resolver for a .tflite model."
        )
    )

    parser.add_argument(
        "model",
        type=Path
    )

    parser.add_argument(
        "output",
        type=Path,
        nargs="?",
        default=Path("include/ModelOps.h")
    )

    args = parser.parse_args()

    if not args.model.is_file():
        raise FileNotFoundError(
            args.model
        )

    operators = read_operators(
        args.model
    )

    print("Operators found:")
    for name in operators:
        print(f"  - {name}")

    header = generate_header(
        operators
    )

    args.output.parent.mkdir(
        parents=True,
        exist_ok=True
    )

    args.output.write_text(
        header,
        encoding="utf-8"
    )

    print()
    print(
        f"Generated: {args.output}"
    )
    print(
        f"Operator count: {len(operators)}"
    )


if __name__ == "__main__":
    main()
