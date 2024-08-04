import { useDispatch } from "react-redux"
import { ConfirmModal } from "../../ConfirmModal"
import Button from 'react-bootstrap/Button';
import { saveConfigAsync } from "./wifiSlice";

const defaultButtonText = 'Save Configuration'
const defaultModalMessage = 'Are you sure you want to save the configuration? The new configuration will be applied when restarting'


export const SaveConfigModalButton = ({buttonText=defaultButtonText, modalText=defaultModalMessage, ...buttonProps}) => {
    const dispatch = useDispatch();
    return <ConfirmModal 
        confirmButton={buttonText}
        text={modalText}
        onConfirm={() => dispatch(saveConfigAsync())}
        RenderButton={(props) => <Button {...props} {...buttonProps} variant='success'>{buttonText}</Button>}
    />

}