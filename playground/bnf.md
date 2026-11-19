<program> = <statement> ; | <statement> ; <program>
<statement> =   print <expression> |
                let <variable> | 
                <variable> = <expression>
<expression> = <term> {(+|-) <expression>}
<term> = <factor> {(*|/|%) <term>}

<factor> = <number> | <variable> | ( <expression> ) | -<factor>
<number> = 0 | [1-9][0-9]*
<variable> = [a-zA-Z_]+






