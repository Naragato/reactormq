function main(option_value, auto_resolver)
    if option_value == true or option_value == "on" or option_value == "yes" or option_value == 1 then
        return true
    end
    if option_value == false or option_value == "off" or option_value == "no" or option_value == 0 then
        return false
    end
    return auto_resolver()
end
