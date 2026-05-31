#!/usr/bin/env bats

@test "adds two integers" {
    run ./examples/bash/testable-calculator.sh add 2 3
    [ "$status" -eq 0 ]
    [ "$output" = "5" ]
}

@test "rejects non-integers" {
    run ./examples/bash/testable-calculator.sh add 2 x
    [ "$status" -eq 2 ]
    [[ "$output" == *"operands must be integers"* ]]
}
